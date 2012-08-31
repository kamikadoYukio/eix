// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <vaeth@mathematik.uni-wuerzburg.de>

#ifndef SRC_CACHE_SQLITE_SQLITE_H_
#define SRC_CACHE_SQLITE_SQLITE_H_ 1

#include <vector>

#include "cache/base.h"

class Category;
class PackageTree;
class TrueIndex;

class SqliteCache : public BasicCache {
	friend int sqlite_callback(void *NotUsed, int argc, char **argv, char **azColName);

	private:
		bool never_add_categories;
		void sqlite_callback_cpp(int argc, const char **argv, const char **azColName) ATTRIBUTE_NONNULL_;
		std::vector<int> trueindex;
		int maxindex;

		/** This variable is actually the this-parameter for our sqlite_callback function.
		    Note that this makes readCategories non-reentrant. */
		static SqliteCache *callback_arg;
		/** Our sqlite_callback() will set this to true in case of an error. */
		bool sqlite_callback_error;
		/** Parameter passing to sqlite_callback() */
		PackageTree *packagetree;
		Category *category;
		const char *cat_name;
		static TrueIndex *true_index;

	public:
		explicit SqliteCache(bool add_categories = false) : BasicCache(), never_add_categories(!add_categories)
		{ }

		bool can_read_multiple_categories() const ATTRIBUTE_CONST_VIRTUAL
		{ return true; }

		bool readCategories(PackageTree *packagetree, const char *catname, Category *cat);

		const char *getType() const
		{
			if(never_add_categories)
				return "sqlite";
			return "sqlite*";
		}
};

#endif  // SRC_CACHE_SQLITE_SQLITE_H_
