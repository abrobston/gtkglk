/*  GtkGlk - Glk Implementation using Gtk+
    Copyright (C) 2003-2004  Evin Robertson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    The author can be reached at evin@users.sf.net  */


/* Based on gtk_paned.c from GTK+ */

#include <gtk/gtk.h>
#include <gtk/gtkpaned.h>
#include "glkpaned.h"

void
gtk_glkpaned_compute_position (GtkPaned *paned,
			    gint      allocation,
			    gint      child1_req,
			    gint      child2_req)
{
  gint old_position;
  
  g_return_if_fail (GTK_IS_PANED (paned));

  old_position = paned->child1_size;

  paned->min_position = paned->child1_shrink ? 0 : child1_req;

  paned->max_position = allocation;
  if (!paned->child2_shrink)
    paned->max_position = MAX (1, paned->max_position - child2_req);

  if (!paned->position_set)
    {
	int percent;
	percent=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(paned), "percent"));
	paned->child1_size = allocation * percent / 100;
    }
  else
    {
      /* If the position was set before the initial allocation.
       * (paned->last_allocation <= 0) just clamp it and leave it.
       */
      if (paned->last_allocation > 0)
	{
	  if (paned->child1_resize && !paned->child2_resize)
	    paned->child1_size += allocation - paned->last_allocation;
	  else if (!(!paned->child1_resize && paned->child2_resize))
	    paned->child1_size = allocation * ((gdouble) paned->child1_size / (paned->last_allocation));
	}
    }

  paned->child1_size = CLAMP (paned->child1_size,
			      paned->min_position,
			      paned->max_position);

  if (paned->child1_size != old_position)
    g_object_notify (G_OBJECT (paned), "position");

  paned->last_allocation = allocation;
}


void gtk_glkpaned_position_changed(GtkPaned *paned)
{
    guint signal_id;
    signal_id = g_signal_lookup("position_changed", G_TYPE_FROM_INSTANCE(paned));

    g_object_set_data(G_OBJECT(paned), "percent", GINT_TO_POINTER(
			  100 * paned->child1_size / paned->max_position));
    
    g_signal_emit(paned, signal_id, 0);
}

gboolean glkpaned_button_release (GtkWidget *widget, GdkEventButton *event)
{
    GtkPaned *paned = GTK_PANED (widget);

    if(paned->in_drag && (event->button == 1)) {
	paned->in_drag = FALSE;
	paned->drag_pos = -1;
	paned->position_set = TRUE;
	gdk_pointer_ungrab (event->time);
	
	gtk_glkpaned_position_changed(paned);
	return TRUE;
    }
    return FALSE;
}

