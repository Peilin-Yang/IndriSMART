#include "indri/Repository.hpp"
#include "indri/CompressedCollection.hpp"
#include "indri/LocalQueryServer.hpp"
#include "indri/ScopedLock.hpp"
#include "indri/QueryEnvironment.hpp"
#include "indri/Parameters.hpp"

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <math.h>


vector<string> read_term_file(const std::string& input_file_path) {
  string line;
  ifstream f (input_file_path.c_str());
  vector<string> l;
  if (f.is_open())
  {
    while ( getline (f, line) )
    {
      l.push_back(line);
    }
    f.close();
  }
  return l;
}


void print_collection_term_weight(indri::collection::Repository& r, const std::string& input_file_path, bool debug=false) {
    vector<string> all_terms = read_term_file(input_file_path);

    indri::server::LocalQueryServer local(r);
    indri::collection::Repository::index_state state = r.indexes();
    for( size_t i=0; i<state->size(); i++ ) {
        indri::index::Index* index = (*state)[i];
        indri::thread::ScopedLock( index->iteratorLock() );

        for ( size_t j=0; j != all_terms.size(); j++ ) {
            std::string term = all_terms[j];
            std::string stem = r.processTerm( term );
            double collection_tf_weight = 0.0;

            indri::index::DocListIterator* iter = index->docListIterator( stem );
            if (iter == NULL) continue;

            iter->startIteration();
            indri::index::DocListIterator::DocumentData* entry;
            
            for( iter->startIteration(); iter->finished() == false; iter->nextEntry() ) {
                entry = iter->currentEntry();
                int local_tf = entry->positions.size();
                collection_tf_weight += pow((log(local_tf) + 1.0), 2);
            }
            delete iter;

            collection_tf_weight = pow(collection_tf_weight, 0.5);
            cout << term << " " << stem << " " << collection_tf_weight << std::endl;
        }
    }
}

void usage() {
    std::cout << "    IndriTextTransformer -index=... -term=... [-debug=0]" << std::endl;
    std::cout << "    1. index - index path" << std::endl;
    std::cout << "    2. file - input file that contains all terms" << std::endl; 
    std::cout << "    3. debug - whether print debug info" << std::endl; 
}

void require_parameter( const char* name, indri::api::Parameters& p ) {
  if( !p.exists( name ) ) {
    usage();
    LEMUR_THROW( LEMUR_MISSING_PARAMETER_ERROR, "Must specify a " + name + " parameter." );
  }
}


int main( int argc, char** argv ) {
  try {
    indri::api::Parameters& parameters = indri::api::Parameters::instance();
    parameters.loadCommandLine( argc, argv );

    require_parameter( "index", parameters );
    require_parameter( "file", parameters );

    bool debug = false;
    if ( parameters.get( "debug", "" ) != "") {
      std::string debug_str = parameters.get( "debug", "" );
      if ( debug_str != "0" ) {
        debug = true;
      } 
    }

    indri::collection::Repository r;
    std::string repName = parameters["index"];
    std::string termFile = parameters["file"];
    
    r.openRead( repName );
    print_collection_term_weight(r, termFile, debug);
    r.close();
    return 0;
  } catch( lemur::api::Exception& e ) {
    LEMUR_ABORT(e);
  }
}


