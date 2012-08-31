// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <vaeth@mathematik.uni-wuerzburg.de>

#ifndef SRC_PORTAGE_CONF_PORTAGESETTINGS_H_
#define SRC_PORTAGE_CONF_PORTAGESETTINGS_H_ 1

#include <map>
#include <set>
#include <string>
#include <vector>

#include "portage/keywords.h"
#include "portage/mask.h"
#include "portage/mask_list.h"
#include "portage/overlay.h"
#include "portage/package.h"

class CascadingProfile;
class EixRc;
class Version;

/* Files for categories the user defined and categories from the official tree */
#define MAKE_GLOBALS_FILE       "/etc/make.globals"
#define MAKE_CONF_FILE          "/etc/make.conf"
#define MAKE_CONF_FILE_NEW      "/etc/portage/make.conf"
#define USER_CATEGORIES_FILE    "/etc/portage/categories"
#define USER_KEYWORDS_FILE1     "/etc/portage/package.keywords"
#define USER_KEYWORDS_FILE2     "/etc/portage/package.accept_keywords"
#define USER_MASK_FILE          "/etc/portage/package.mask"
#define USER_UNMASK_FILE        "/etc/portage/package.unmask"
#define USER_USE_FILE           "/etc/portage/package.use"
#define USER_ENV_FILE           "/etc/portage/package.env"
#define USER_LICENSE_FILE       "/etc/portage/package.license"
#define USER_CFLAGS_FILE        "/etc/portage/package.cflags"
#define USER_PROFILE_DIR        "/etc/portage/profile"
#define PORTDIR_CATEGORIES_FILE "profiles/categories"
#define PORTDIR_MASK_FILE       "profiles/package.mask"
#define PORTDIR_MAKE_DEFAULTS   "profiles/make.defaults"

class PortageSettings;

class PortageUserConfig {
		friend class PortageSettings;
	private:
		PortageSettings      *m_settings;
		MaskList<Mask>        m_localmasks;
		MaskList<KeywordMask> m_accept_keywords;
		MaskList<KeywordMask> m_use, m_env, m_license, m_cflags;
		bool read_use, read_env, read_license, read_cflags;

		/** Your cascading profile, including local settings */
		CascadingProfile     *profile;

		/** return true if something was added */
		bool readMasks();
		bool readKeywords();

		bool CheckList(Package *p, const MaskList<KeywordMask> *list, Keywords::Redundant flag_double, Keywords::Redundant flag_in) const ATTRIBUTE_NONNULL_;
		bool CheckFile(Package *p, const char *file, MaskList<KeywordMask> *list, bool *readfile, Keywords::Redundant flag_double, Keywords::Redundant flag_in) const ATTRIBUTE_NONNULL_;
		static void ReadVersionFile(const char *file, MaskList<KeywordMask> *list) ATTRIBUTE_NONNULL_;

		void pushback_set_accepted_keywords(std::vector<std::string> *result, const Version *v) const ATTRIBUTE_NONNULL_;

	public:
		PortageUserConfig(PortageSettings *psettings, CascadingProfile *local_profile) ATTRIBUTE_NONNULL_;

		~PortageUserConfig();

		void setProfileMasks(Package *p) const ATTRIBUTE_NONNULL_;

		/// @return true if something from /etc/portage/package.* applied and check involves masks
		bool setMasks(Package *p, Keywords::Redundant check = Keywords::RED_NOTHING, bool file_mask_is_profile = false) const ATTRIBUTE_NONNULL_;
		/// @return true if something from /etc/portage/package.* applied and check involves keywords
		bool setKeyflags(Package *p, Keywords::Redundant check = Keywords::RED_NOTHING) const ATTRIBUTE_NONNULL_;

		/// @return true if something from /etc/portage/package.use applied
		bool CheckUse(Package *p, Keywords::Redundant check) ATTRIBUTE_NONNULL_
		{
			if(check & Keywords::RED_ALL_USE)
				return CheckFile(p, USER_USE_FILE, &m_use, &read_use, check & Keywords::RED_DOUBLE_USE, check & Keywords::RED_IN_USE);
			return false;
		}
		/// @return true if something from /etc/portage/package.env applied
		bool CheckEnv(Package *p, Keywords::Redundant check) ATTRIBUTE_NONNULL_
		{
			if(check & Keywords::RED_ALL_ENV)
				return CheckFile(p, USER_ENV_FILE, &m_env, &read_env, check & Keywords::RED_DOUBLE_ENV, check & Keywords::RED_IN_ENV);
			return false;
		}
		/// @return true if something from /etc/portage/package.license applied
		bool CheckLicense(Package *p, Keywords::Redundant check) ATTRIBUTE_NONNULL_
		{
			if(check & Keywords::RED_ALL_LICENSE)
				return CheckFile(p, USER_LICENSE_FILE, &m_license, &read_license, check & Keywords::RED_DOUBLE_LICENSE, check & Keywords::RED_IN_LICENSE);
			return false;
		}
		/// @return true if something from /etc/portage/package.cflags applied
		bool CheckCflags(Package *p, Keywords::Redundant check) ATTRIBUTE_NONNULL_
		{
			if(check & Keywords::RED_ALL_CFLAGS)
				return CheckFile(p, USER_CFLAGS_FILE, &m_cflags, &read_cflags, check & Keywords::RED_DOUBLE_CFLAGS, check & Keywords::RED_IN_CFLAGS);
			return false;
		}
};

/** Holds Portage's settings, e.g. masks, categories, overlay paths.
 * Reads needed files if content is requested .. so don't worry about initialization :) */
class PortageSettings : public std::map<std::string, std::string> {
	private:
		friend class CascadingProfile;
		friend class PortageUserConfig;

		EixRc *settings_rc;
		std::vector<std::string> m_categories; /**< Vector of all allowed categories. */
		std::vector<std::string> m_accepted_keywords;
		std::set<std::string>    m_accepted_keywords_set, m_arch_set,
		                         m_plain_accepted_keywords_set,
		                        *m_local_arch_set, *m_auto_arch_set;
		std::string              m_raised_arch;

		MaskList<SetMask>        m_package_sets;

		std::vector<SetsList>    parent_sets;
		std::vector<SetsList>    children_sets;

		/** One may argue whether reading the settings for the upgrade policy
		/ * is only a cache, but it makes things simpler if we say so */
		mutable bool know_upgrade_policy, upgrade_policy;
		mutable MaskList<Mask> upgrade_policy_exceptions;

		mutable bool know_expands;
		mutable std::map<std::string, std::string> expand_vars;

		bool know_world_sets;
		std::vector<std::string> world_sets;
		bool world_setslist_up_to_date;
		SetsList world_setslist;

		/** Your cascading profile, excluding local settings */
		CascadingProfile  *profile;

		void override_by_env(const char **vars) ATTRIBUTE_NONNULL_;
		void read_config(const std::string &name, const std::string &prefix);

		void addOverlayProfiles(CascadingProfile *p) const ATTRIBUTE_NONNULL_;

		void calc_recursive_sets(Package *p) const ATTRIBUTE_NONNULL_;

		void read_world_sets(const char *file) ATTRIBUTE_NONNULL_;
		void calc_world_sets(Package *p) ATTRIBUTE_NONNULL_;

		void update_world_setslist();

		const MaskList<SetMask> *getPackageSets() const
		{ return &m_package_sets; }

	public:
		bool m_recurse_sets;
		std::string m_eprefix;
		std::string m_eprefixconf;
		std::string m_eprefixprofile;
		std::string m_eprefixportdir;
		std::string m_eprefixoverlays;
		std::string m_eprefixaccessoverlays;
		std::string m_world;

		PortageUserConfig *user_config;

		RepoList repos;
		std::vector<std::string> set_names;

#ifndef HAVE_SETENV
		bool export_portdir_overlay;
#endif
		using std::map<std::string, std::string>::operator[];

		const std::string &operator[](const std::string &var) const ATTRIBUTE_PURE;

		const char *cstr(const std::string &var) const ATTRIBUTE_PURE;

		/** Read make.globals and make.conf. */
		PortageSettings(EixRc *eixrc, bool getlocal, bool init_world);

		/** Free memory. */
		~PortageSettings();

		std::string resolve_overlay_name(const std::string &path, bool resolve);

		void add_repo(std::string *path, bool resolve, bool modify_path = false);

		void add_repo_vector(std::vector<std::string> *v, bool resolve, bool modify_v = false) ATTRIBUTE_NONNULL_;

		void store_world_sets(const std::vector<std::string> *s_world_sets, bool override = false);

		void get_setnames(std::set<std::string> *names, const Package *p, bool also_nonlocal = false) const ATTRIBUTE_NONNULL_;

		std::string get_setnames(const Package *p, bool also_nonlocal = false) const;

		void read_local_sets(const std::vector<std::string> &dir_list);

		const std::vector<std::string> *get_world_sets() const
		{ return &world_sets; }

		/** pushback categories from profiles to vec. Categories may be duplicate.
		    Result is not cashed, i.e. this should be called only once. */
		void pushback_categories(std::vector<std::string> *vec) ATTRIBUTE_NONNULL_;

		void setMasks(Package *p, bool filemask_is_profile = false) const ATTRIBUTE_NONNULL_;

		/// Set stability according to arch or local ACCEPTED_KEYWORDS
		void setKeyflags(Package *pkg, bool use_accepted_keywords) const ATTRIBUTE_NONNULL_;

		void add_name(SetsList *l, const std::string &s, bool recurse) const ATTRIBUTE_NONNULL_;

		bool calc_allow_upgrade_slots(const Package *p) const ATTRIBUTE_NONNULL_;

		void calc_local_sets(Package *p) const ATTRIBUTE_NONNULL_;

		void finalize(Package *p) ATTRIBUTE_NONNULL_
		{
			calc_world_sets(p);
			p->finalize_masks();
		}

		void get_effective_keywords_profile(Package *p) const ATTRIBUTE_NONNULL_;

		void get_effective_keywords_userprofile(Package *p) const ATTRIBUTE_NONNULL_;

		bool use_expand(std::string *var, std::string *expvar, const std::string &value) const ATTRIBUTE_NONNULL_;

		static void init_static();
};

#endif  // SRC_PORTAGE_CONF_PORTAGESETTINGS_H_
