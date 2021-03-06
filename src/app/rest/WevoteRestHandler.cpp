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
               [this]( http_request msg ) { _fullPipeline( msg ); });
    _addRoute( Method::POST  ,  wevoteClassifer ,
               [this]( http_request msg ) { _wevoteClassifier( msg ); });
    _addRoute( Method::POST  ,  abundance ,
               [this]( http_request msg ) { _generateProfile( msg ); });
}

void WevoteRestHandler::_wevoteClassifier( http_request message )
{
    auto task =
            message.reply(status_codes::OK)
            .then( [this,message](){
        message.extract_json()
                .then([this,message]( web::json::value value )
        {
            WevoteSubmitEnsemble submission =
                    io::DeObjectifier::fromObject< WevoteSubmitEnsemble >( value );

            _classifier.classify( submission.getReadsInfo() ,
                                  submission.getMinNumAgreed() ,
                                  submission.getPenalty());

            submission.getDistances() =
                    io::mapToValues( _classifier.algorithmsAccumulativeDistances(
                                         submission.getReadsInfo() ,
                                         submission.getAlgorithms()));


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

void WevoteRestHandler::_fullPipeline(http_request message)
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

            /// Trick: sort back algorithms to client as the script use lexicographical ordering.
            std::sort( submission.getAlgorithms().begin() , submission.getAlgorithms().end());

            WevoteScriptHandler pipelineHandler;

            submission.getReadsInfo() = pipelineHandler
                    .execute( submission.getSequences() , submission.getAlgorithms());

            _classifier.classify( submission.getReadsInfo() ,
                                  submission.getMinNumAgreed() ,
                                  submission.getPenalty());

            submission.getDistances() =
                    io::mapToValues( _classifier.algorithmsAccumulativeDistances(
                                         submission.getReadsInfo() ,
                                         submission.getAlgorithms()));


            uint32_t undefined =
                    std::count_if( submission.getReadsInfo().cbegin() , submission.getReadsInfo().cend() ,
                                   []( const wevote::ReadInfo &read )
            {
                return read.resolvedTaxon == wevote::ReadInfo::noAnnotation;
            });
            LOG_INFO("Unresolved taxan=%d/%d",undefined,submission.getReadsInfo().size());

            std::map< uint32_t , wevote::TaxLine > annotatedTaxa = _annotator
                    .annotateTaxonomyLines( submission.getReadsInfo() );

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

void WevoteRestHandler::_generateProfile( http_request message )
{
    auto task =
            message.reply( status_codes::OK )
            .then( [this,message](){
        message.extract_json()
                .then([this,message]( web::json::value value )
        {

            LOG_DEBUG("Generating profile ..");
            WevoteSubmitEnsemble submission =
                    io::DeObjectifier::fromObject< WevoteSubmitEnsemble >( value );

            std::map< uint32_t , wevote::TaxLine > annotatedTaxa = _annotator
                    .annotateTaxonomyLines( submission.getReadsInfo() );

            for( auto it = annotatedTaxa.cbegin() ; it != annotatedTaxa.cend() ; ++it )
                submission.getAbundanceProfile().push_back( it->second );

            submission.getStatus().setPercentage( 100.0 );
            submission.getStatus().setCode( Status::StatusCode::SUCCESS );


            LOG_DEBUG("Transmitting..");
            _transmitJSON( submission );
            LOG_DEBUG("[DONE] Transmitting[job:%d] .." , _jobCounter.load());
            _jobCounter++;
            LOG_DEBUG("[DONE] Submiting job:%d..",_jobCounter.load());
            LOG_DEBUG("[DONE] Generating profile ..");
        });
    });

    try{
        task.wait();
    } catch(std::exception &e){
        LOG_DEBUG("%s",e.what());
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


}  // namespace rest
}  // namespace wevote


