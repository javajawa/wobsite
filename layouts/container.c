#include "layouts/container.h"
#include "layouts/template.h"

const union html_node container = element( "div",
	NULL,
	children(
		yield( 1 )
	)
);
