// Copyright 2012 Yandex Artem Babenko
#include <iostream>

#include "boost/program_options.hpp"

#include "indexer.h"

using namespace boost::program_options;

/**
 * Number of threads for indexing
 */
int THREADS_COUNT;
/**
 * Type, should be BVEC or FVEC
 */
PointType point_type;
/**
 * Number of coordinates in a point
 */
Dimensions SPACE_DIMENSION;
/**
 * File with vocabularies for multiindex structure
 */
string coarse_vocabs_file;
/**
 * File with vocabularies for reranking
 */
string fine_vocabs_file;
/**
 * File with points to index
 */
string points_file;
/**
 * File with points metainfo (imageId, etc.)
 */
string metainfo_file;
/**
 * Reranking approach, should be USE_RESIDUALS or USE_INIT_POINTS
 */
RerankMode mode;
/**
 * Common prefix of all multiindex files
 */
string files_prefix;
/**
 * Should we calculate coarse quantizations (they can be precomputed)
 */
bool build_coarse_quantizations;
/**
 * File with points coarse quantizations
 */
string coarse_quantizations_file;
/**
 * How many points should we index
 */
int points_count;
/**
 * Multiplicity of multiindex
 */
int multiplicity;


/**
 * File to write report in
 */
string report_file;

bool SetOptions(int argc, char** argv) {
  options_description description("Options");
  description.add_options()
    ("threads_count,t", value<int>())
    ("multiplicity,m", value<int>())
    ("points_file,p", value<string>())
    ("metainfo_file,z", value<string>())
    ("coarse_vocabs_file,c", value<string>()->required(), "set coarse_vocabs_file")
    ("fine_vocabs_file,f", value<string>())
    ("input_point_type,i", value<string>())
    ("build_coarse,b", bool_switch(), "Flag B")
    ("use_residuals,r", bool_switch(), "Flag R")
    ("points_count,o", value<int>())
    ("coarse_quantization_file,q", value<string>())
    ("space_dim,d", value<int>())
    ("files_prefix,_", value<string>());
  variables_map name_to_value;
  try {
    store(command_line_parser(argc, argv).options(description).run(), name_to_value);
  } catch (const invalid_command_line_syntax& inv_syntax) {
    switch (inv_syntax.kind()) {
      case invalid_syntax::missing_parameter :
        cout << "Missing argument for option '" << inv_syntax.tokens() << "'.\n";
        break;
      default:
        cout << "Syntax error, kind " << int(inv_syntax.kind()) << "\n";
        break;
      };
    return false;
  } catch (const unknown_option& unkn_option) {
    cout << "Unknown option '" << unkn_option.get_option_name() << "'\n";
    return false;
  }
  if (name_to_value.count("help")) {
    cout << description << "\n";
    return false;
  }

  if (name_to_value.count("threads_count"))
      THREADS_COUNT =              name_to_value["threads_count"].as<int>();
  if (name_to_value.count("multiplicity"))
      multiplicity =               name_to_value["multiplicity"].as<int>();
  if (name_to_value.count("points_file"))
      points_file =                name_to_value["points_file"].as<string>();
  if (name_to_value.count("metainfo_file"))
      metainfo_file =              name_to_value["metainfo_file"].as<string>();
  if (name_to_value.count("coarse_vocabs_file"))
      coarse_vocabs_file =         name_to_value["coarse_vocabs_file"].as<string>();
  if (name_to_value.count("fine_vocabs_file"))
      fine_vocabs_file =           name_to_value["fine_vocabs_file"].as<string>();
  if (name_to_value.count("space_dim"))
      SPACE_DIMENSION =            name_to_value["space_dim"].as<int>();
  if (name_to_value.count("files_prefix"))
      files_prefix =               name_to_value["files_prefix"].as<string>();
  if (name_to_value.count("points_count"))
      points_count =               name_to_value["points_count"].as<int>();
 
  build_coarse_quantizations = (name_to_value["build_coarse"].as<bool>() == true) ? true : false;
  mode = name_to_value["use_residuals"].as<bool>() == true ? USE_RESIDUALS : USE_INIT_POINTS;

  if (name_to_value.find("coarse_quantization_file") != name_to_value.end()) {
    coarse_quantizations_file =  name_to_value["coarse_quantization_file"].as<string>();
  }
  if (name_to_value.count("query_point_type"))
  {
      if (name_to_value["input_point_type"].as<string>() == "FVEC") {
          point_type = FVEC;
      }
      else if (name_to_value["input_point_type"].as<string>() == "BVEC") {
          point_type = BVEC;
      }
  }


  try
  {
      notify(name_to_value);
  }
  catch (std::exception& e)
  {
      std::cerr << "Error: " << e.what() << "\n";
      return false;
  }
  catch (...)
  {
      std::cerr << "Unknown error!" << "\n";
      return false;
  }

  return true;

}

int main(int argc, char** argv) 
{
    bool result = SetOptions(argc, argv);
    if (!result)
        return 1;

  cout << "Options are set ...\n";
  vector<Centroids> coarse_vocabs;
  vector<Centroids> fine_vocabs;
  ReadVocabularies<float>(coarse_vocabs_file, SPACE_DIMENSION, &coarse_vocabs);
  ReadFineVocabs<float>(fine_vocabs_file, &fine_vocabs);
  cout << "Vocs are read ...\n";
  if(fine_vocabs.size() == 8) {
    MultiIndexer<RerankADC8> indexer(multiplicity);
    indexer.BuildMultiIndex(points_file, metainfo_file, points_count, coarse_vocabs, 
                            fine_vocabs, mode, build_coarse_quantizations,
                            files_prefix, coarse_quantizations_file);
  } else if(fine_vocabs.size() == 16) {
    MultiIndexer<RerankADC16> indexer(multiplicity);
    indexer.BuildMultiIndex(points_file, metainfo_file, points_count, coarse_vocabs, 
                            fine_vocabs, mode, build_coarse_quantizations,
                            files_prefix, coarse_quantizations_file);  
  }
  return 0;
}