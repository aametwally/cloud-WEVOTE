#include <QDir>
#include <QFile>

#include "headers.hpp"
#include "helpers.hpp"
#include "Colors.hh"

#include "WevoteClassifier.h"
#include "TaxonomyBuilder.h"

#include "CommandLineParser.hpp"

#define DEFAULT_NUM_THREADS 1
#define DEFAULT_SCORE 0
#define DEFAULT_MIN_NUM_AGREED 0
#define DEFAULT_PERNALTY 2


struct WevoteParameters
{
    bool verbose;
    std::string query;
    std::string prefix;
    std::string taxonomyDB;
    uint32_t score;
    int threads;
    int penalty;
    uint32_t minNumAgreed;
    std::string toString() const
    {
        std::stringstream stream;
        stream << "[verbose:" << verbose << "]"
               << "[input:" << query << "]"
               << "[prefix:" << prefix << "]"
               << "[taxonomyDB:" << taxonomyDB << "]"
               << "[score:" << score << "]"
               << "[threads:" << threads << "]"
               << "[penalty:" << penalty << "]"
               << "[minNumAgreed:" << minNumAgreed << "]";
        return stream.str();
    }
    WevoteParameters()
        : threads{1}, penalty{2}, minNumAgreed{0}, score{0}, verbose{false}
    {}
};

const std::list< QCommandLineOption > commandLineOptions =
{
    {
        QStringList() << "i" << "input-file",
        "Input ensemble file produced by the used algorithms."  ,
        "input-file"
    },
    {
        QStringList() << "d" <<  "taxonomy-db-path",
        "The path of the taxonomy database file." ,
        "taxonomy-db-path"
    },
    {
        QStringList() << "p" <<  "output-prefix",
        "OutputFile Prefix" ,
        "output-prefix"
    },
    {
        QStringList() << "n" <<  "threads",
        "Num of threads." ,
        "threads" ,
        QString::number( DEFAULT_NUM_THREADS )
    },
    {
        QStringList() << "k" <<  "penalty",
        "Penalty." ,
        "penalty",
        QString::number( DEFAULT_PERNALTY )
    },
    {
        QStringList() << "a" <<  "min-num-agreed",
        "Minimum number of tools agreed tools on WEVOTE decision." ,
        "min-num-agreed",
        QString::number( DEFAULT_MIN_NUM_AGREED )
    },
    {
        QStringList() << "s" <<  "score",
        "Score threshold." ,
        "score",
        QString::number( DEFAULT_SCORE )
    },
    {
        QStringList() << "v" <<  "verbose",
        "Enable verbose mode." ,
        "verbose",
        QString::number( 1 )
    }
};

auto extractFunction = []( const QCommandLineParser &parser ,
        ParsingResults<WevoteParameters> &results )
{
    /// parse commandline arguments
    if( !parser.isSet("input-file") ||
            !QFile::exists( parser.value("input-file")))
    {
        results.success = CommandLineResult::CommandLineError;
        results.errorMessage = "Input file is not specified or "
                               "does not exists.";
        return;
    }
    if( !parser.isSet("taxonomy-db-path"))
    {
        results.success = CommandLineResult::CommandLineError;
        results.errorMessage = "Taxonomy file is not specified.";
        return;
    }
    QDir taxonomyDBPath( parser.value("taxonomy-db-path"));
    if( !taxonomyDBPath.exists())
    {
        results.success = CommandLineResult::CommandLineError;
        results.errorMessage = "Taxonomy provided dir does not exist.";
        return;
    }
    if( !parser.isSet("output-prefix"))
    {
        results.success = CommandLineResult::CommandLineError;
        results.errorMessage = "Output prefix is not specified.";
        return;
    }
    results.parameters.verbose =
            static_cast< bool >(parser.value("verbose").toShort());
    results.parameters.query =
            parser.value("input-file").toStdString();
    results.parameters.taxonomyDB =
            parser.value("taxonomy-db-path").toStdString();
    results.parameters.prefix =
            parser.value("output-prefix").toStdString();
    results.parameters.penalty =
            parser.value("penalty").toInt();
    results.parameters.score =
            parser.value("score").toUInt();
    results.parameters.threads =
            parser.value("threads").toInt();
    results.parameters.minNumAgreed =
            parser.value("min-num-agreed").toUInt();
};

int main(int argc, char *argv[])
{
    QCoreApplication a( argc , argv );
    CommandLineParser cmdLineParser( a , commandLineOptions ,
                                     std::string(argv[0]) + " help" );
    ParsingResults<WevoteParameters> parsingResults;
    cmdLineParser.process();
    cmdLineParser.tokenize( extractFunction , parsingResults );

    if( parsingResults.success == CommandLineResult::CommandLineError )
        LOG_ERROR("Command line error: %s", parsingResults.errorMessage.c_str());
    else
        LOG_DEBUG("parameters: %s", parsingResults.parameters.toString().c_str());

    const auto &param = parsingResults.parameters;

    const std::string nodesFilename=
            QDir( QString::fromStdString( param.taxonomyDB ))
            .filePath("nodes.dmp").toStdString();

    const std::string namesFilename=
            QDir( QString::fromStdString( param.taxonomyDB ))
            .filePath("names.dmp").toStdString();

    const std::string outputDetails=
            param.prefix + "_WEVOTE_Details.txt";

    const std::string outputDetailsCSV=
            param.prefix + "_WEVOTE_Details.csv";

    LOG_INFO("NodesFilename=%s", nodesFilename.c_str());
    LOG_INFO("NamesFilename=%s", namesFilename.c_str());

    /// Read CSV formated input file
    LOG_INFO("Loading reads..");
    auto reads = wevote::WevoteClassifier::getUnclassifiedReads( param.query );
    LOG_INFO("[DONE] Loading reads..");

    /// Build taxonomy trees
    LOG_INFO("Building Taxonomy..");
    wevote::TaxonomyBuilder taxonomy( nodesFilename , namesFilename );
    LOG_INFO("[DONE] Building Taxonomy..");

    wevote::WevoteClassifier wevoteClassifier( taxonomy );
    wevoteClassifier.classify( reads.first ,
                               param.minNumAgreed , param.penalty ,
                               wevote::WevoteClassifier::manhattanDistance() ,
                               param.threads );

    uint32_t undefined =
            std::count_if( reads.first.cbegin() , reads.first.cend() ,
                           []( const wevote::ReadInfo &read )
    {
        return read.resolvedTaxon == wevote::ReadInfo::noAnnotation;
    });
    LOG_INFO("Unresolved taxan=%d/%d",undefined,reads.first.size());

    /// Output.
    wevote::WevoteClassifier::writeResults( reads.first , reads.second , outputDetails );
    wevote::WevoteClassifier::writeResults( reads.first , reads.second , outputDetailsCSV , true );

    printf("\n");
    for( const std::pair< std::string , double > &algorithmCost :
         wevoteClassifier.algorithmsAccumulativeDistances( reads.first , reads.second ))
        printf( STD_WHITE  "<%s>\n\t\t"  STD_RESET
                STD_YELLOW "total-distance:%f\n"   STD_RESET ,
                algorithmCost.first.c_str() ,
                algorithmCost.second ) ;

    return EXIT_SUCCESS;
}
