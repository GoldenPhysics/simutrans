/*
 * The trees builder
 */

#ifndef gui_baum_edit_h
#define gui_baum_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"


class tree_desc_t;
class tool_plant_tree_t;

class baum_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_plant_tree_t baum_tool;
	static char param_str[256];

	const tree_desc_t *desc;

	vector_tpl<const tree_desc_t *>tree_list;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	baum_edit_frame_t(player_t* player_);

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "baum builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char* get_help_filename() const { return "baum_build.txt"; }
};

#endif
