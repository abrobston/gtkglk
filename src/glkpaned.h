/* Nasty #defines so that we don't have to modify gtk?paned.[ch] very much,
   so when it changes it's easy to re-patch to make it still work */

#define GtkVPaned GtkVGlkPaned
#define _GtkVPaned _GtkVGlkPaned
#define GtkVPanedClass GtkVGlkPanedClass
#define _GtkVPanedClass _GtkVGlkPanedClass
#define GtkHPaned GtkHGlkPaned
#define _GtkHPaned _GtkHGlkPaned
#define GtkHPanedClass GtkHGlkPanedClass
#define _GtkHPanedClass _GtkHGlkPanedClass

#define gtk_vpaned_get_type gtk_vglkpaned_get_type
#define gtk_hpaned_get_type gtk_hglkpaned_get_type
#define gtk_vpaned_new gtk_vglkpaned_new
#define gtk_hpaned_new gtk_hglkpaned_new

#define gtk_vpaned_class_init gtk_vglkpaned_class_init
#define gtk_vpaned_init gtk_vglkpaned_init
#define gtk_vpaned_size_request gtk_vglkpaned_size_request
#define gtk_vpaned_size_allocate gtk_vglkpaned_size_allocate
#define gtk_hpaned_class_init gtk_hglkpaned_class_init
#define gtk_hpaned_init gtk_hglkpaned_init
#define gtk_hpaned_size_request gtk_hglkpaned_size_request
#define gtk_hpaned_size_allocate gtk_hglkpaned_size_allocate

#define gtk_paned_compute_position gtk_glkpaned_compute_position


#include "gtkhpaned.h"
#include "gtkvpaned.h"
#include "gglk.h"

void
gtk_glkpaned_compute_position (GtkPaned *paned,
			       gint      allocation,
			       gint      child1_req,
			       gint      child2_req) PRIVATE;

void gtk_glkpaned_position_changed(GtkPaned *paned) PRIVATE;
gboolean glkpaned_button_release (GtkWidget *widget, GdkEventButton *event) PRIVATE;

