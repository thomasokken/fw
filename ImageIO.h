///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

#ifndef IMAGEFILE_H
#define IMAGEFILE_H 1

class FWPixmap;
class Map;
class Iterator;

class ImageIO {
    private:
        static Map *map;

    public:
        ImageIO();
        virtual ~ImageIO();

        // Note: all data returned through pointer-to-pointer arguments
        // becomes the caller's responsibility and must be freed using
        // free().

        virtual const char *name() = 0;
        virtual bool can_read(const char *filename) = 0;
        virtual bool read(const char *filename, char **plugin_name,
                          void **plugin_data, int *plugin_data_length,
                          FWPixmap *pm, char **message) = 0;
        virtual bool write(const char *filename, const char *plugin_name,
                           const void *plugin_data, int plugin_data_length,
                           const FWPixmap *pm, char **message) = 0;

        static void regist(ImageIO *imgio);
        static Iterator *list();
        static bool sread(const char *filename, char **type,
                          char **plugin_name, void **plugin_data,
                          int *plugin_data_length, FWPixmap *pm,
                          char **message);
        static bool swrite(const char *filename, const char *type,
                           const char *plugin_name, const void *plugin_data,
                           int plugin_data_length, const FWPixmap *pm,
                           char **message);
};

#endif
