#! /usr/bin/env sh

export LC_ALL=C

to_classname () {
	local i="$1" j
	case "$1" in
		flat|port*2.0|port*2.0.*)     i=port2_0_0_;;
		backport|port*2.1|port*2.1.0) i=port2_1_0_;;
		port*2.1.2|port*2.1\*)        i=port2_1_2_;;
	esac
	j="${i#?}"
	echo -n "${i%${j}}" | tr 'a-z' 'A-Z'
	echo "${j}Cache"
}

cache_stars="parse ebuild"
extra_caches="metadata sqlite cdb $cache_stars"
cache_includes="eixcache port2_0_0 port2_1_0 port2_1_2 $extra_caches"
cache_names="$extra_caches
	portage-2.0 portage-2.0.51 flat
	portage-2.1 portage-2.1.0 backport"
[ "${1}" = "portage-2.1.2" ] && cache_names="$cache_names
	portage-2.1.2 portage-2.1*"

cat<<END
// AUTOGENERATED BY MAKE .. DO NOT EDIT!
END

for cache_name in $cache_includes; do
	cat<<END
#include "$cache_name/$cache_name.h"
END
done

cat<<END
using namespace std;

BasicCache *get_cache(const string &name) {
END

for cache_name in $cache_names; do
	cache_class=`to_classname "$cache_name"`
	cat<<END
	if (name == "$cache_name")
		return new $cache_class;
END
done
for cache_name in $cache_stars; do
	cache_class=`to_classname "$cache_name"`
	cat<<END
	if (name == "${cache_name}*")
		return new $cache_class(true);
END
done
cat<<END
	if (name == "parse|ebuild")
		return new ParseCache(false, true);
	if ((name == "parse|ebuild*") || (name == "mixed"))
		return new ParseCache(false, true, true);
	if (name == "parse*|ebuild")
		return new ParseCache(true, true);
	if (name == "parse*|ebuild*")
		return new ParseCache(true, true, true);
	EixCache *e=new EixCache;
	if(e->initialize(name))
		return e;
	delete e;
	return NULL;
}
END
