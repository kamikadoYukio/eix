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

#include "config.h"

#include "portagesettings.h"

#include <fstream>
#include <portage/mask.h>

#include <eixTk/utils.h>
#include <eixTk/stringutils.h>

#include <varsreader.h>

bool grab_masks(const char *file, Mask::Type type, MaskList *cat_map, vector<Mask*> *mask_vec)
{
	ifstream mask_file(file);
	if(mask_file.is_open()) {
		string line;
		while(getline(mask_file, line)) {
			trim(&line);
			if(line.size() == 0 || line[0] == '#')
				continue;
			try {
				Mask *m = new Mask(line, type);
				OOM_ASSERT(m);
				if(cat_map) {
					cat_map->add(m);
				}
				else {
					mask_vec->push_back(m);
				}
			}
			catch(ExBasic e) {
				cerr << "-- Invalid line in " << file << ": \"" << line << "\"" << endl
				     << "   " << e.getMessage() << endl;
			}
		}
		mask_file.close();
		return true;
	}
	return false;
}

/** Key that should accumelate their content rathern then replace. */
static const char *default_accumulating_keys[] = {
	"USE",
	"CONFIG_*",
	"FEATURES",
	"ACCEPT_KEYWORDS",
	NULL
};

/** Read make.globals and make.conf. */
PortageSettings::PortageSettings()
{
	profile     = new CascadingProfile(this);
	user_config = new PortageUserConfig(this);

	VarsReader make_globals(VarsReader::SUBST_VARS|VarsReader::INTO_MAP|VarsReader::APPEND_VALUES);
	make_globals.accumulatingKeys(default_accumulating_keys);
	make_globals.useMap(this);
	make_globals.read("/etc/make.globals");

	VarsReader make_conf(VarsReader::SUBST_VARS|VarsReader::INTO_MAP|VarsReader::APPEND_VALUES);
	make_conf.accumulatingKeys(default_accumulating_keys);
	make_conf.useMap(this);
	make_conf.read("/etc/make.conf");

	if((*this)["PORTDIR"].size() == 0 ) {
		(*this)["PORTDIR"] = "/usr/portage/";
	}
	else {
		(*this)["PORTDIR"].append("/");
	}

	overlays = split_string((*this)["PORTDIR_OVERLAY"]);

	if(getenv("ACCEPT_KEYWORDS")) {
		(*this)["ACCEPT_KEYWORDS"].append(string(" ") + getenv("ACCEPT_KEYWORDS"));
	}
	
	_accepted_keyword = split_string((*this)["ACCEPT_KEYWORDS"]);
	_accepted_keyword = resolve_plus_minus(_accepted_keyword);
	(*this)["ACCEPT_KEYWORDS"] = join_vector(_accepted_keyword);
	_accepted_keywords.set((*this)["ARCH"], (*this)["ACCEPT_KEYWORDS"]);
}
		
PortageSettings::~PortageSettings()
{
	if(profile) {
		delete profile;
	}
	if(user_config) {
		delete user_config;
	}
}

/** Return vector of all possible all categories.
 * Reads categories on first call. */
vector<string> *PortageSettings::getCategories()
{
	if(_categories.size() == 0) {
		/* Merge categories from /etc/portage/categories and
		 * portdir/profile/categories */
		pushback_lines(USER_CATEGORIES_FILE, &_categories);
		pushback_lines(((*this)["PORTDIR"] + PORTDIR_CATEGORIES_FILE).c_str(), &_categories);
		sort(_categories.begin(), _categories.end());
		unique(_categories.begin(),_categories.end());
	}
	return &_categories;
}

/** Read maskings & unmaskings as well as user-defined ones */
MaskList *PortageSettings::getMasks()
{
	if(_masks.size() == 0) {
		if(!grab_masks(string((*this)["PORTDIR"]+"profiles/package.mask").c_str(), Mask::maskMask, &_masks) )
			WARNING("Can't read %sprofiles/package.mask\n", (*this)["PORTDIR"].c_str());
	}
	return &(_masks);
}


bool PortageUserConfig::readKeywords() {
	/* Prepend a ~ to every token. 
	 * FIXME: So do we only care for ARCH or do we also care for ACCEPT_KEYWORDS? */
	vector<string> splitted = split_string((*_psettings)["ARCH"], "\t \n\r");
	for(vector<string>::iterator it = splitted.begin(); it != splitted.end(); ++it) {
		if(strchr("-~", (*it)[0]) == NULL) {
			*it = "~" + *it;
		}
	}
	string fscked_arch = join_vector(splitted);

	ifstream mask_file("/etc/portage/package.keywords");
	if(mask_file.is_open()) {
		string line;
		while(getline(mask_file, line)) {
			trim(&line);
			if(line.size() == 0 || line[0] == '#')
				continue;
			try {
				string::size_type i = line.find_first_of("\t ");
				if(i == string::npos) {
					KeywordMask *m = new KeywordMask(line, Mask::maskTypeNone);
					m->keywords = fscked_arch;
					_keywords.add(m);
				}
				else {
					KeywordMask *m = new KeywordMask(line.substr(0, i), Mask::maskTypeNone);
					m->keywords = line.substr(i + 1);
					_keywords.add(m);
				}
			}
			catch(ExBasic e) {
				cerr << "-- Invalid line in /etc/portage/package.keywords: \"" << line << "\"" << endl
				     << "   " << e.getMessage() << endl;
			}
		}
		mask_file.close();
		return true;
	}
	return false;
}

void PortageUserConfig::setMasks(Package *p) {
	/* Set hardmasks */
	for(MaskList::viterator it = _mask.get(p)->begin(); it != _mask.get(p)->end(); ++it) {
		(*it)->checkMask(*p, false, false);
	}
}

static inline void apply_keywords(Version &v, Keywords::Type t) {
	if(v.get() & t) {
		v |= Keywords::KEY_STABLE;
	}
	else {
		v &= (~Keywords::KEY_STABLE | ~Keywords::KEY_ALL);
	}
}

void PortageUserConfig::setStability(Package *p, Keywords kw) {
	vector<Mask*>        *keyword_masks =  _keywords.get(p);
	map<Version*,string>  sorted_by_versions;

	vector<Mask*>::iterator  it = keyword_masks->begin();
	if(it != keyword_masks->end())
	{
		for(;
			it != keyword_masks->end();
			++it) 
		{
			vector<Version*> matches = (*it)->getMatches(*p);
			for(vector<Version*>::iterator  v = matches.begin();
				v != matches.end();
				++v) 
			{
				sorted_by_versions[*v].append(" " + ((KeywordMask*) *it)->keywords);
			}
		}

#if 0
		// HU? I shouldn't take crack ..
		for(map<Version*,string>::iterator i = sorted_by_versions.begin();
			i != sorted_by_versions.end();
			++i)
		{
		}
#endif
	}

	string arch = (*_psettings)["ARCH"];

	for(Package::iterator i = p->begin();
		i != p->end();
		++i)
	{
		Keywords lkw(kw.get());

		string skw = sorted_by_versions[*i];
		if(skw.size() > 0)
		{
			vector<string> kv = split_string(skw);
			kv.erase(unique(kv.begin(), kv.end()), kv.end());

			for(vector<string>::iterator kvi = kv.begin();
				kvi != kv.end();
				++kvi)
			{
				if(*kvi == "-" + arch) {
					lkw &= (~Keywords::KEY_STABLE | ~Keywords::KEY_ALL);
				}
				else if(*kvi == "~" + arch) {
					lkw |= Keywords::KEY_UNSTABLE;
				}
				else if(*kvi == "-~" + arch) {
					lkw &= (~Keywords::KEY_UNSTABLE | ~Keywords::KEY_ALL);
				}
				else if(*kvi == "-*") {
					lkw |= Keywords::KEY_MINUSASTERISK;
				}
			}

		}
		apply_keywords(**i, lkw.get());
	}
}
