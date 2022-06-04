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

#ifndef PROCESSING_H_
#define PROCESSING_H_

#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <memory>
#include <string_view>

#include "arguments.h"
#include "crypto.h"

#if defined(__WINDOWS__) || defined(__WIN32__) || defined(__WIN64__) || defined(_WIN64) || defined(_WIN32) || defined(__WINNT) || defined(__WINNT__)
    constexpr std::string_view PATH_SEPARATOR = "\\";
#else
    constexpr std::string_view PATH_SEPARATOR = "/";
#endif /* __WIN32__ */

enum class file_type
{
    BLOCK,
    REGULAR,
    DIR,
    UNSUPPORTED,
    ERROR
};

constexpr const size_t BUFFER_SIZE  = 0x1000;

typedef struct context_t
{
    uint8_t *buf        = nullptr;
    FILE *output        = nullptr;
} context_t;

file_type stat_file(const std::string & path)
{
    struct stat sb;
    if (::stat(path.c_str(), &sb) != 0)
    {
        fprintf(stderr, "Can not process '%s': IO error=%d\n", path.c_str(), int(errno));
        return file_type::ERROR;
    }

    // Decode file type
    switch (sb.st_mode & S_IFMT) {
        case S_IFBLK:  return file_type::BLOCK;
        case S_IFREG:  return file_type::REGULAR;
        case S_IFDIR:  return file_type::DIR;
        case S_IFIFO:
        case S_IFLNK:
        case S_IFSOCK:
        case S_IFCHR:
        default:
            break;
    }

    return file_type::UNSUPPORTED;
}

std::string gen_temporary_name(const std::string & file)
{
    size_t i = 0;
    struct stat sb;

    while (true)
    {
        std::string temp = file + "." + std::to_string(i++) + ".tmp";
        if (::stat(temp.c_str(), &sb) != 0)
        {
            if (errno == ENOENT)
                return temp;
        }
    }
}

bool read_directory(std::vector<std::string> & elements, const std::string & path)
{
    struct dirent *de;
    DIR *d = opendir(path.c_str());
    if (d == nullptr)
    {
        fprintf(stderr, "Error accessing directory '%s': error=%d", path.c_str(), int(errno));
        return false;
    }

    while ((de = readdir(d)) != nullptr)
    {
        std::string item = path + std::string(PATH_SEPARATOR) + de->d_name;
        elements.emplace_back(std::move(item));
    }
    closedir(d);

    return true;
}

bool write_fully(context_t & ctx, FILE *out, ssize_t count)
{
    for (ssize_t offset = 0; offset < count; )
    {
        ssize_t written = fwrite(ctx.buf, sizeof(uint8_t), count - offset, out);
        if (written <= 0)
            return false;
        offset += written;
    }

    return true;
}

status process_data(context_t & ctx, FILE *out, FILE *in, const settings_t & settings)
{
    Crypto crypto(settings.key);

    while (!feof(in))
    {
        // Read block of data
        ssize_t read = fread(ctx.buf, sizeof(uint8_t), BUFFER_SIZE, in);
        if (read <= 0)
        {
            fprintf(stderr, "Error reading file: error=%d", int(errno));
            return status::IO_ERROR;
        }

        // Process the block
        for (ssize_t i=0; i<read; ++i)
            ctx.buf[i] ^= crypto.gen();

        // Seek target file for the specific offset for inplace processing
        if (out == in)
        {
            if (fseek(out, -read, SEEK_CUR) < 0)
            {
                fprintf(stderr, "Error seeking file: error=%d", int(errno));
                return status::IO_ERROR;
            }
        }

        // Write fully block data
        if (ctx.output)
        {
            if (!write_fully(ctx, ctx.output, read))
            {
                fprintf(stderr, "Error writing output file: error=%d", int(errno));
                return status::IO_ERROR;
            }
        }
        if (out)
        {
            if (!write_fully(ctx, out, read))
            {
                fprintf(stderr, "Error writing file: error=%d", int(errno));
                return status::IO_ERROR;
            }
        }
    }

    return status::OK;
}

status process_regular_file(context_t & ctx, const std::string & path, const settings_t & settings)
{
    if (settings.verbose)
        fprintf(stderr, "Processing file '%s'\n", path.c_str());

    status res = status::OK;

    if ((settings.to_stdout) || (ctx.output != nullptr))
    {
        // Inplace processing of file
        FILE *in = fopen(path.c_str(), "rb");
        if (in == nullptr)
        {
            fprintf(stderr, "Error reading file '%s': error=%d", path.c_str(), int(errno));
            return status::IO_ERROR;
        }

        FILE * out = (settings.to_stdout) ? stdout : nullptr;
        res = process_data(ctx, out, in, settings);

        if (fclose(in) != 0)
        {
            fprintf(stderr, "Error writing file '%s': error=%d", path.c_str(), int(errno));
            res = status::IO_ERROR;
        }
    }
    else if (settings.inplace)
    {
        // Inplace processing of file
        FILE *fd = fopen(path.c_str(), "rb+");
        if (fd == nullptr)
        {
            fprintf(stderr, "Error writing file '%s': error=%d", path.c_str(), int(errno));
            return status::IO_ERROR;
        }

        res = process_data(ctx, fd, fd, settings);

        if (fclose(fd) != 0)
        {
            fprintf(stderr, "Error writing file '%s': error=%d", path.c_str(), int(errno));
            res = status::IO_ERROR;
        }
    }
    else
    {
        // Process data to temporary file
        const std::string temp = gen_temporary_name(path);

        FILE *in = fopen(path.c_str(), "rb");
        if (in != nullptr)
        {
            FILE *out = fopen(temp.c_str(), "wb");
            if (out != nullptr)
            {
                res = process_data(ctx, out, in, settings);

                if (fclose(out) != 0)
                {
                    fprintf(stderr, "Error writing file '%s': error=%d", path.c_str(), int(errno));
                    res = status::IO_ERROR;
                }
            }

            if (fclose(in) != 0)
            {
                fprintf(stderr, "Error reading file '%s': error=%d", path.c_str(), int(errno));
                res = status::IO_ERROR;
            }
        }
        else
        {
            fprintf(stderr, "Error reading file '%s': error=%d", path.c_str(), int(errno));
            res = status::IO_ERROR;
        }

        // Replace the previously used file with the new file
        if (rename(path.c_str(), temp.c_str()) != 0)
        {
            // If rename failed, remove temporary file
            unlink(temp.c_str());

            fprintf(stderr, "Error replacing file '%s': error=%d", path.c_str(), int(errno));
            res = status::IO_ERROR;
        }
    }

    return res;
}

status process_item(context_t & ctx, const std::string & path, const settings_t & settings)
{
    const file_type type = stat_file(path);
    switch (type)
    {
        case file_type::REGULAR:
            return process_regular_file(ctx, path, settings);
        case file_type::BLOCK:
            if (!settings.inplace)
            {
                fprintf(stderr, "Can not process '%s': block device requires in-place mode\n", path.c_str());
                return status::BAD_STATE;
            }
            return process_regular_file(ctx, path, settings);
        case file_type::DIR:
        {
            if (!settings.recursive)
            {
                fprintf(stderr, "Can not process '%s': is a directory\n", path.c_str());
                return status::BAD_STATE;
            }

            // Read directory
            std::vector<std::string> elements;
            if (!read_directory(elements, path))
                return status::IO_ERROR;

            // Process each element
            for (const std::string & element: elements)
            {
                status res = process_item(ctx, element, settings);
                if (res != status::OK)
                    return res;
            }

            return status::OK;
        }
        case file_type::UNSUPPORTED:
            fprintf(stderr, "Can not process '%s': unsupported device type\n", path.c_str());
            return status::BAD_STATE;
        default:
            break;
    }

    return status::IO_ERROR;
}


status process_files(const settings_t & settings)
{
    status res = status::OK;

    // Initialize context
    context_t ctx;
    std::unique_ptr<uint8_t> buf(new uint8_t[BUFFER_SIZE]);
    ctx.buf     = buf.get();

    if (settings.output)
    {
        ctx.output = fopen(settings.output->c_str(), "wb");
        if (ctx.output == nullptr)
        {
            fprintf(stderr, "Error writing file '%s': error=%d", settings.output->c_str(), int(errno));
            res = status::IO_ERROR;
        }
    }

    // Perform processing
    if (!settings.paths.empty())
    {
        // Process list of files
        for (const std::string & item : settings.paths)
        {
            res = process_item(ctx, item, settings);
            if (res != status::OK)
                break;
        }
    }
    else // Process STDIN and send result to STDOUT
        res = process_data(ctx, stdout, stdin, settings);

    // Close output file if present
    if (ctx.output != nullptr)
    {
        if (fclose(ctx.output) != 0)
        {
            fprintf(stderr, "Error writing '%s': error=%d", settings.output->c_str(), int(errno));
            res = status::IO_ERROR;
        }
        ctx.output = nullptr;
    }

    return res;
}


#endif /* PROCESSING_H_ */
