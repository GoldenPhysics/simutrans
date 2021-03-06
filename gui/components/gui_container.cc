/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * A container for other gui_components. Is itself
 * a gui_component, and can therefor be nested.
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include "gui_container.h"
#include "../gui_theme.h"

gui_container_t::gui_container_t() : gui_component_t(), comp_focus(NULL)
{
	list_dirty = false;
	inside_infowin_event = false;
}

/**
 * Returns the minimum rectangle which encloses all children
 * @author Max Kielland
 */
scr_rect gui_container_t::get_min_boundaries() const
{
	scr_rect client_bound;

	FOR( slist_tpl<gui_component_t*>, const c, components ) {
		client_bound.outer_bounds( scr_rect( c->get_pos(), c->get_size().w, c->get_size().h ) );
	}
	return client_bound;
}


/**
 * Add component to the container
 * @author Hj. Malthaner
 */
void gui_container_t::add_component(gui_component_t *comp)
{
	/* Inserts/builds the dialog from bottom to top:
	 * Essential for combo-boxes, so they overlap lower elements
	 */
	components.insert(comp);
	list_dirty = true;
}


/**
 * Remove/destroy component from container
 * @author Hj. Malthaner
 */
void gui_container_t::remove_component(gui_component_t *comp)
{
	/* since we can remove a subcomponent,
	 * that actually contains the element with focus
	 */
	if(  comp_focus == comp->get_focus()  ) {
		comp_focus = NULL;
	}
	components.remove(comp);
	list_dirty = true;
}


/**
 * Remove all components from container
 * @author Markus Weber
 */
void gui_container_t::remove_all()
{
	// clear also focus
	while(  !components.empty()  ) {
		remove_component( components.remove_first() );
	}
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_container_t::infowin_event(const event_t *ev)
{
	inside_infowin_event = true;

	bool swallowed = false;
	gui_component_t *new_focus = comp_focus;

	// need to change focus?
	if(  ev->ev_class==EVENT_KEYBOARD  ) {

		if(  comp_focus  ) {
			event_t ev2 = *ev;
			translate_event(&ev2, -comp_focus->get_pos().x, -comp_focus->get_pos().y);
			swallowed = comp_focus->infowin_event(&ev2);
		}

		// Knightly : either event not swallowed, or inner container has no focused child component after TAB event
		if(  !swallowed  ||  (ev->ev_code==SIM_KEY_TAB  &&  comp_focus  &&  comp_focus->get_focus()==NULL)  ) {
			if(  ev->ev_code==SIM_KEY_TAB  ) {
				// TAB: find new focus
				new_focus = NULL;
				if(  !IS_SHIFT_PRESSED(ev)  ) {
					// find next textinput field
					FOR(slist_tpl<gui_component_t*>, const c, components) {
						if (c == comp_focus) break;
						if (c->is_focusable()) {
							new_focus = c;
						}
					}
				}
				else {
					// or previous input field
					bool valid = comp_focus==NULL;
					FOR(slist_tpl<gui_component_t*>, const c, components) {
						if (valid && c->is_focusable()) {
							new_focus = c;
							break;
						}
						if (c == comp_focus) {
							valid = true;
						}
					}
				}

				// Knightly :	inner containers with focusable components may not have a focused component yet
				//				==> give the inner container a chance to activate the first focusable component
				if(  new_focus  &&  new_focus->get_focus()==NULL  ) {
					event_t ev2 = *ev;
					translate_event(&ev2, -new_focus->get_pos().x, -new_focus->get_pos().y);
					new_focus->infowin_event(&ev2);
				}

				swallowed = comp_focus!=new_focus;
			}
			else if(  ev->ev_code==SIM_KEY_ENTER  ||  ev->ev_code==SIM_KEY_ESCAPE  ) {
				new_focus = NULL;
				if(  ev->ev_code==SIM_KEY_ESCAPE  ) {
					// no untop message even!
					comp_focus = NULL;
				}
				swallowed = comp_focus!=new_focus;
			}
		}
	}
	else {
		// CASE : not a keyboard event
		const int x = ev->ev_class==EVENT_MOVE ? ev->mx : ev->cx;
		const int y = ev->ev_class==EVENT_MOVE ? ev->my : ev->cy;

		slist_tpl<gui_component_t *>handle_mouseover;
		FOR(  slist_tpl<gui_component_t*>,  const comp,  components  ) {
			if(  list_dirty  ) {
				break;
			}

			// Hajo: deliver events if
			// a) The mouse or click coordinates are inside the component
			// b) The event affects all components, this are WINDOW events
			if(  comp  ) {
				if(  DOES_WINDOW_CHILDREN_NEED( ev )  ) { // (Mathew Hounsell)
					// Hajo: no need to translate the event, it has no valid coordinates either
					comp->infowin_event(ev);
				}
				else if(  comp->is_visible()  ) {
					if(  comp->getroffen(x, y)  ) {
						handle_mouseover.insert( comp );
					}
				}

			} // if(comp)
		}

		/* since the last drawn are overlaid over all others
		 * the event-handling must go reverse too
		 */
		FOR(  slist_tpl<gui_component_t*>,  const comp,  handle_mouseover  ) {
			if (list_dirty) {
				break;
			}

			// Hajo: if component hit, translate coordinates and deliver event
			event_t ev2 = *ev;
			translate_event(&ev2, -comp->get_pos().x, -comp->get_pos().y);

			// CAUTION : call to infowin_event() should not delete the component itself!
			swallowed = comp->infowin_event(&ev2);

			// focused component of this container can only be one of its immediate children
			gui_component_t *focus = comp->get_focus() ? comp : NULL;

			// set focus for component, if component allows focus
			if(  focus  &&  IS_LEFTCLICK(ev)  &&  comp->getroffen(ev->cx, ev->cy)  ) {
				/* the focus swallow all following events;
				 * due to the activation action
				 */
				new_focus = focus;
			}
			// stop here, if event swallowed or focus received
			if(  swallowed  ||  comp==new_focus  ) {
				break;
			}
		}
	}

	list_dirty = false;

	// handle unfocus/next focus stuff
	if(  new_focus!=comp_focus  ) {
		gui_component_t *old_focus = comp_focus;
		comp_focus = new_focus;
		if(  old_focus  ) {
			// release focus
			event_t ev2 = *ev;
			translate_event(&ev2, -old_focus->get_pos().x, -old_focus->get_pos().y);
			ev2.ev_class = INFOWIN;
			ev2.ev_code = WIN_UNTOP;
			old_focus->infowin_event(&ev2);
		}
	}

	inside_infowin_event = false;

	return swallowed;
}


/* Draw the component
 * @author Hj. Malthaner
 */
void gui_container_t::draw(scr_coord offset)
{
	const scr_coord screen_pos = pos + offset;

	// For debug purpose, draw the container's boundary
	// display_ddd_box_rgb(screen_pos.x,screen_pos.y,get_size().w, get_size().h, color_idx_to_rgb(COL_RED), color_idx_to_rgb(COL_RED), true);

	FOR(slist_tpl<gui_component_t*>, const c, components) {
		if (c->is_visible()) {
			// @author hsiegeln; check if component is hidden or displayed
			c->draw(screen_pos);
		}
	}
}


bool gui_container_t::is_focusable()
{
	FOR( slist_tpl<gui_component_t*>, const c, components ) {
		if(  c->is_focusable()  ) {
			return true;
		}
	}
	return false;
}


void gui_container_t::set_focus( gui_component_t *c )
{
	if(  inside_infowin_event  ) {
		dbg->error("gui_container_t::set_focus", "called from inside infowin_event, will have no effect");
	}
	if(  components.is_contained(c)  ||  c==NULL  ) {
		comp_focus = c;
	}
}


/**
 * returns element that has the focus
 * that is: go down the hierarchy as much as possible
 */
gui_component_t *gui_container_t::get_focus()
{
	// if the comp_focus-element has another focused element
	// .. return this element instead
	return comp_focus ? comp_focus->get_focus() : NULL;
}
