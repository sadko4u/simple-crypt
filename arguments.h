/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of simple-crypt
 * Created on: 4 июн. 2022 г.
 *
 * simple-crypt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * simple-crypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with simple-crypt. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_

#include <optional>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdint.h>

#include "crypto.h"

enum class status
{
    OK,
    USAGE,
    BAD_ARGUMENTS,
    NO_KEY,
    IO_ERROR,
    BAD_STATE
};

typedef struct settings_t
{
    key_hash_t key          = 0;
    bool verbose            = false;
    bool recursive          = false;
    bool inplace            = false;
    bool to_stdout          = false;
    std::optional<std::string>  output = std::nullopt;
    std::vector<std::string>    paths;
} settings_t;


status parse_arguments(settings_t & settings, int argc, const char **argv)
{
    bool key_set = false;

    for (int i=1; i<argc;)
    {
        const std::string arg = argv[i++];
        if ((arg == "-v") || (arg == "--verbose"))
            settings.verbose = true;
        else if ((arg == "-r") || (arg == "--recursive"))
             settings.recursive = true;
        else if ((arg == "-d") || (arg == "--dump"))
            settings.to_stdout = true;
        else if ((arg == "-o") || (arg == "--output"))
        {
            if (i >= argc)
            {
                fprintf(stderr, "Output file not specified.\n");
                return status::BAD_ARGUMENTS;
            }
            settings.output = std::string(argv[i++]);
        }
        else if ((arg == "-h") || (arg == "--help"))
            return status::USAGE;
        else if ((arg == "-i") || (arg == "--inplace"))
            settings.inplace = true;
        else if ((arg == "-k") || (arg == "--key"))
        {
            if (i >= argc)
            {
                fprintf(stderr, "Key value not specified.\n");
                return status::BAD_ARGUMENTS;
            }
            settings.key = hash_key(std::string(argv[i++]));
            key_set = true;
        }
        else
            settings.paths.push_back(arg);
    }

    // Verify setting of different arguments
    if (!key_set)
    {
        fprintf(stderr, "Key value not provided.\n");
        return status::NO_KEY;
    }

    return status::OK;
}

void print_usage(int argc, const char **argv)
{
    const char *name = (argc > 0) ? argv[0] : "simple-crypt";

    printf("USAGE: %s [args...] [files...]\n\n", name);
    printf("Available arguments:\n");
    printf("  -d, --dump              Output encrypted content to stdout instead of file\n");
    printf("  -h, --help              Output usage\n");
    printf("  -i, --inplace           Overwrite file immediately, do not use temporary files\n");
    printf("  -k, --key               Specify encryption key\n");
    printf("  -o, --output            Specify output file to write data\n");
    printf("  -r, --recursive         Process directories recursively\n");
    printf("  -v, --verbose           Output name of processed files\n");
}


#endif /* ARGUMENTS_H_ */
