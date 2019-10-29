#ifndef GRID_MODULE_H_INCLUDED
#define GRID_MODULE_H_INCLUDED

#include "sam.h"
#include <stdlib.h>



#include "../../grid_lib/grid_sys.h"
#include "../../grid_lib/grid_buf.h"


#include "../../grid_lib/grid_ui.h"


#include "../../grid_modules/grid_module_bu16_revb.h"
#include "../../grid_modules/grid_module_po16_revb.h"




void grid_port_process_ui(GRID_PORT_t* por);



void grid_module_init(void);


#include "grid_module.c"

#endif
