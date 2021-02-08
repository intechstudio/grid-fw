/*
 * grid_debug.h
 *
 * Created: 3/5/2020 11:24:14 AM
 *  Author: WPC-User
 */ 


#ifndef GRID_DEBUG_H_
#define GRID_DEBUG_H_



#define GRID_DEBUG_PRINT_ENABLE


#ifdef GRID_DEBUG_PRINT_ENABLE
#define GRID_DEBUG_PRINT(context, ...) do{ printf( __VA_ARGS__ ); } while( false )
#else
#define GRID_DEBUG_PRINT(context, ...) do{ } while ( false )
#endif



#define GRID_DEBUG_TRAP(context, ...) do{ GRID_DEBUG_PRINT(context, __VA_ARGS__ ); delay_ms(1000);} while( true )

#define GRID_DEBUG_WARNING(context, ...) do{ GRID_DEBUG_PRINT(context, "{\"type\":\"WARNING\", \"data\": [\"%s\"]}\r\n",  __VA_ARGS__ ); } while( false )
#define GRID_DEBUG_LOG(context, ...)	 do{ GRID_DEBUG_PRINT(context, "{\"type\":\"LOG\", \"data\": [\"%s\"]}\r\n",  __VA_ARGS__ ); } while( false )
	
	
	
	
	

enum grid_debug_context{
	
	GRID_DEBUG_CONTEXT_BOOT,
	GRID_DEBUG_CONTEXT_PORT,
	GRID_DEBUG_CONTEXT_MAIN
	
};


enum grid_debug_type{
	
	GRID_DEBUG_TYPE_LOG,
	GRID_DEBUG_TYPE_WARNING,
	GRID_DEBUG_TYPE_ERROR,
	GRID_DEBUG_TYPE_TRAP
	
};



#endif /* GRID_DEBUG_H_ */