#ifndef UTILS_H
#define UTILS_H

#include <boost/program_options.hpp>
#include "../Header_Files/message.h"

namespace po = boost::program_options;

void configure_cmdline_options(po::options_description& desc);

po::options_description configure_help_options();

void validate_place_order(const po::variables_map& vm);

jsonrpc store_required_values(const po::variables_map& vm,jsonrpc j);

#endif // UTILS_H