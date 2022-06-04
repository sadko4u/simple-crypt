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

#include "arguments.h"
#include "processing.h"

int main(int argc, const char **argv)
{
    settings_t settings;
    status res = parse_arguments(settings, argc, argv);
    if (res != status::OK)
    {
        print_usage(argc, argv);
        return (res == status::USAGE) ? 0 : int(res);
    }

    return int(process_files(settings));
}

