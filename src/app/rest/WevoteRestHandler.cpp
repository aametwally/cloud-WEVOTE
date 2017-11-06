#include "WevoteRestHandler.h"

namespace wevote
{
namespace rest
{

std::atomic_uint WevoteRestHandler::_jobCounter{0};
WevoteRestHandler::WevoteRestHandler( const uri &uri ,
                                      const TaxonomyBuilder &taxonomy )
    : RestHandler( uri ), _taxonomy( taxonomy ) ,
      _classifier( _taxonomy ) , _annotator( _taxonomy )
{

}

void WevoteRestHandler::_addRoutes()
{
    const auto fullPipeline =
            U("/wevote/submit/fullpipeline");
    const auto wevoteClassifer =
            U("/wevote/submit/ensemble");
    const auto abundance =
            U("/wevote/submit/abundance");
    _addRoute( Method::POST  ,  fullPipeline ,
               [this]( http_request msg ) { _receiveUnclassifiedSequences( msg ); });
    _addRoute( Method::POST  ,  wevoteClassifer ,
               [this]( http_request msg ) { _receiveEnsembleClassifiedSequences( msg ); });
    _addRoute( Method::POST  ,  abundance ,
               [this]( http_request msg ) { _receiveClassifiedSequences( msg ); });
}

void WevoteRestHandler::_receiveEnsembleClassifiedSequences(http_request message)
{
    auto task =
            message.reply(status_codes::OK)
            .then( [this,message](){
        message.extract_json()
                .then([this,message]( web::json::value value )
        {
            WevoteSubmitEnsemble submission =
                    io::DeObjectifier::fromObject< WevoteSubmitEnsemble >( value );

            submission.getDistances() =
                    _classifier.classify( submission.getReadsInfo() , submission.getMinNumAgreed() ,
                                          submission.getPenalty());

            uint32_t undefined =
                    std::count_if( submission.getReadsInfo().cbegin() , submission.getReadsInfo().cend() ,
                                   []( const wevote::ReadInfo &read )
            {
                return read.resolvedTaxon == wevote::ReadInfo::noAnnotation;
            });
            LOG_INFO("Unresolved taxan=%d/%d",undefined,submission.getReadsInfo().size());

            std::map< uint32_t , wevote::TaxLine > annotatedTaxa =
                    _annotator.annotateTaxonomyLines( submission.getReadsInfo() );

            for( auto it = annotatedTaxa.cbegin() ; it != annotatedTaxa.cend() ; ++it )
                submission.getAbundanceProfile().push_back( it->second );

            submission.getStatus().setPercentage( 100.0 );
            submission.getStatus().setCode( Status::StatusCode::SUCCESS );


            LOG_DEBUG("Transmitting..");
            _transmitJSON( submission );
            LOG_DEBUG("[DONE] Transmitting[job:%d] .." , _jobCounter.load());
            _jobCounter++;
            LOG_DEBUG("[DONE] Submiting job:%d..",_jobCounter.load());
        });
    });

    try{
        task.wait();
    } catch(std::exception &e){
        LOG_DEBUG("%s",e.what());
    }
}

void WevoteRestHandler::_receiveUnclassifiedSequences(http_request message)
{
    auto task =
            message.reply( status_codes::OK )
            .then( [this,message](){
        message.extract_json()
                .then([this,message]( web::json::value value )
        {
            LOG_DEBUG("Full pipeline ..");
            WevoteSubmitEnsemble submission =
                    io::DeObjectifier::fromObject< WevoteSubmitEnsemble >( value );

            submission.getReadsInfo() =
                    _fullpipeline( submission );
            submission.getDistances() =
                    _classifier.classify( submission.getReadsInfo() , submission.getMinNumAgreed() ,
                                          submission.getPenalty());

            uint32_t undefined =
                    std::count_if( submission.getReadsInfo().cbegin() , submission.getReadsInfo().cend() ,
                                   []( const wevote::ReadInfo &read )
            {
                return read.resolvedTaxon == wevote::ReadInfo::noAnnotation;
            });
            LOG_INFO("Unresolved taxan=%d/%d",undefined,submission.getReadsInfo().size());

            std::map< uint32_t , wevote::TaxLine > annotatedTaxa =
                    _annotator.annotateTaxonomyLines( submission.getReadsInfo() );

            for( auto it = annotatedTaxa.cbegin() ; it != annotatedTaxa.cend() ; ++it )
                submission.getAbundanceProfile().push_back( it->second );

            submission.getStatus().setPercentage( 100.0 );
            submission.getStatus().setCode( Status::StatusCode::SUCCESS );


            LOG_DEBUG("Transmitting..");
            _transmitJSON( submission );
            LOG_DEBUG("[DONE] Transmitting[job:%d] .." , _jobCounter.load());
            _jobCounter++;
            LOG_DEBUG("[DONE] Submiting job:%d..",_jobCounter.load());
            LOG_DEBUG("[DONE] Full pipeline ..");
        });
    });

    try{
        task.wait();
    } catch(std::exception &e){
        LOG_DEBUG("%s",e.what());
    }
}

void WevoteRestHandler::_receiveClassifiedSequences( http_request message )
{
    auto task =
            message.reply( status_codes::OK )
            .then( [this,message](){
        message.extract_json()
                .then([this,message]( web::json::value value )
        {
            LOG_DEBUG("Full pipeline ..");
            WevoteSubmitEnsemble submission =
                    io::DeObjectifier::fromObject< WevoteSubmitEnsemble >( value );

            std::map< uint32_t , wevote::TaxLine > annotatedTaxa =
                    _annotator.annotateTaxonomyLines( submission.getReadsInfo() );

            for( auto it = annotatedTaxa.cbegin() ; it != annotatedTaxa.cend() ; ++it )
                submission.getAbundanceProfile().push_back( it->second );

            submission.getStatus().setPercentage( 100.0 );
            submission.getStatus().setCode( Status::StatusCode::SUCCESS );


            LOG_DEBUG("Transmitting..");
            _transmitJSON( submission );
            LOG_DEBUG("[DONE] Transmitting[job:%d] .." , _jobCounter.load());
            _jobCounter++;
            LOG_DEBUG("[DONE] Submiting job:%d..",_jobCounter.load());
            LOG_DEBUG("[DONE] Full pipeline ..");
        });
    });

    try{
        task.wait();
    } catch(std::exception &e){
        LOG_DEBUG("%s",e.what());
    }
}

std::vector< ReadInfo > WevoteRestHandler::_fullpipeline( const WevoteSubmitEnsemble &submission )
{
    const std::string id = std::to_string( _getId());
    const std::string seperator = "/";
    const std::string executableDirectory =
            qApp->applicationDirPath().toStdString();
    const std::string pipelineScript =
            executableDirectory + seperator + WEVOTE_PIPELINE_SCRIPT_NAME;
    const std::string queryFile =
            executableDirectory + seperator +  id  + "_query.fa";
    const std::string outPrefix =
            executableDirectory + seperator +  id  + "_out";
    const std::string outputFile =
            outPrefix + seperator +  id  + "_out_ensemble.csv";
    const unsigned int threadsCount =
            (std::thread::hardware_concurrency() < 2 )?
                DEFAULT_THREADS_COUNT : std::thread::hardware_concurrency();

    io::flushStringToFile( io::join( submission.getSequences() , std::string("\n")) , queryFile );

    std::string arguments = " --input " + queryFile;
    arguments += " --output " + outPrefix;
    arguments += " --threads " + std::to_string( threadsCount );
    std::for_each( submission.getAlgorithms().cbegin() , submission.getAlgorithms().cend() ,
                   [&arguments]( const std::string &algorithm ){
        std::string algorithmLowerCase = algorithm;
        std::transform(algorithmLowerCase.begin(), algorithmLowerCase.end(),
                       algorithmLowerCase.begin(), ::tolower);
        if( std::find( wevote::config::algorithms.cbegin() ,
                       wevote::config::algorithms.cend() ,
                       algorithmLowerCase ) != wevote::config::algorithms.cend())
            arguments += " --" + algorithmLowerCase;
    });
    const std::string cmd = pipelineScript + arguments;
    LOG_DEBUG("Executing:%s",cmd.c_str());
    int status = std::system( cmd.c_str());
    LOG_DEBUG("[DONE] Executing:%s", cmd.c_str());

    QFile::remove( QString::fromStdString( queryFile ));

    if( status == EXIT_SUCCESS )
    {
        const std::vector< std::string > unclassifiedReads = io::getFileLines( outputFile );
        auto reads = ReadInfo::parseUnclassifiedReads( unclassifiedReads.cbegin() , unclassifiedReads.cend());
        QFile::remove( QString::fromStdString( outputFile ));
        return std::move( reads.first );
    }
    else
    {
        LOG_DEBUG("Failure executing command:%s", cmd.c_str());
        return {};
    }
}

void WevoteRestHandler::_transmitJSON( const WevoteSubmitEnsemble &data )
{
    LOG_DEBUG("Submitting..");
    LOG_DEBUG("serializing results..");
    json::value dataJSON = io::Objectifier::objectFrom( data );
    LOG_DEBUG("[DONE] serializing results [job:%d] .." , _jobCounter.load());

    const RemoteAddress address = data.getResultsRoute();

    http::http_request request( methods::POST );
    request.set_body( dataJSON );
    try{
        _getClient( address ).request(request)
                .then([](http_response response)-> pplx::task<json::value>{
            return response.extract_json();
            ;})
        .then([](pplx::task<json::value> previousTask){
            try{
                const json::value & v = previousTask.get();
                LOG_DEBUG("%s" , USTR( v.serialize()));
            } catch(const std::exception &e){
                LOG_DEBUG("%s", e.what());
            }
        })
        .wait();
    } catch(std::exception &e){
        LOG_DEBUG("%s",e.what());
    }
}

uint WevoteRestHandler::_getId()
{
    return _jobCounter++;
}


}
}


