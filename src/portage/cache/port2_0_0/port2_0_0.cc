/***************************************************************************
 *   eix is a small utility for searching ebuilds in the                   *
 *   Gentoo Linux portage system. It uses indexing to allow quick searches *
 *   in package descriptions with regular expressions.                     *
 *                                                                         *
 *   https://sourceforge.net/projects/eix                                  *
 *                                                                         *
 *   Copyright (c)                                                         *
 *     Wolfgang Frisch <xororand@users.sourceforge.net>                    *
 *     Emil Beinroth <emilbeinroth@gmx.net>                                *
 *     Martin V�th <vaeth@mathematik.uni-wuerzburg.de>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "port2_0_0.h"
#include <cache-utils/flat-reader.h>

#include <portage/packagetree.h>
#include <portage/package.h>
#include <portage/version.h>

#include <string.h>
#include <dirent.h>

#include <config.h>

/* Path to portage cache */
#define PORTAGE_CACHE_PATH "/var/cache/edb/dep"

using namespace std;

static int cachefiles_selector (SCANDIR_ARG3 dent)
{
	return (dent->d_name[0] != '.'
			&& strchr(dent->d_name, '-') != 0);
}

bool Port2_0_0_Cache::readCategory(Category &vec) throw(ExBasic)
{
	string catpath = m_prefix + PORTAGE_CACHE_PATH + m_scheme + vec.name();
	struct dirent **dents;
	int numfiles = scandir(catpath.c_str(), &dents, cachefiles_selector, alphasort);
	char **aux = NULL;

	for(int i=0; i<numfiles;)
	{
		Version *version;
		Version *newest = NULL;

		/* Split string into package and version, and catch any errors. */
		aux = ExplodeAtom::split(dents[i]->d_name);
		if(aux == NULL)
		{
			m_error_callback("Can't split '%s' into package and version.", dents[i]->d_name);
			++i;
			continue;
		}

		/* Search for existing package */
		Package *pkg = vec.findPackage(aux[0]);

		/* If none was found create one */
		if(pkg == NULL)
			pkg = vec.addPackage(aux[0]);

		do {
			/* Make version and add it to package. */
			version = new Version(aux[1]);

			/* Read stability from cachefile */
			string keywords, iuse;
			flat_get_keywords_slot_iuse(catpath + "/" + dents[i]->d_name, keywords, version->slot, iuse, m_error_callback);
			version->set(m_arch, keywords);
			version->set_iuse(iuse);
			version->overlay_key = m_overlay_key;

			pkg->addVersion(version);
			if(*(pkg->latest()) == *version)
				newest=version;

			/* Free old split */
			free(aux[0]);
			free(aux[1]);
			memset(aux, '\0', sizeof(char*) * 2);

			/* If this is the last file we break so we can get the full
			 * information after this while-loop. If we still have more files
			 * ahead we can just read the next file. */
			if(++i == numfiles)
				break;

			/* Split new filename into package and version, and catch any errors. */
			aux = ExplodeAtom::split(dents[i]->d_name);
			if(!aux) {
				m_error_callback("Can't split %s into package and version.", dents[i]->d_name);
				break;
			}
		} while(strcmp(aux[0], pkg->name.c_str()) == 0);
		if(aux)
		{
			free(aux[0]);
			free(aux[1]);
		}

		/* Read the cache file of the last version completely */
		if(newest) // provided we have read the "last" version
			flat_read_file(string(catpath + "/" + pkg->name + "-" + newest->getFull()).c_str(), pkg, m_error_callback);
	}

	if(numfiles > 0)
	{
		for(int i=0; i<numfiles; i++ )
			free(dents[i]);
		free(dents);
	}
	return true;
}