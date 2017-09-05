/*
 * Copyright (c) 2003
 *    Motoyuki Kasahara
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef TINYEB_APPENDIX_H
#define TINYEB_APPENDIX_H

#include "ebtiny/eb.h"

#define EB_Appendix                     EB_Book
#define eb_bind_appendix                eb_bind
#define eb_initialize_appendix          eb_initialize_book
#define eb_finalize_appendix            eb_finalize_book
#define eb_appendix_subbook_list        eb_subbook_list
#define eb_appendix_subbook_directory2  eb_subbook_directory2
#define eb_load_all_appendix_subbooks	eb_load_all_subbooks

#endif /* not TINYEB_APPENDIX_H */
