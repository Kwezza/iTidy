
#line 6 "stdint.h"
 typedef signed char int8_t ;
 typedef signed short int16_t ;
 typedef signed long int32_t ;
 typedef signed long long int64_t ;









 typedef unsigned char uint8_t ;
 typedef unsigned short uint16_t ;
 typedef unsigned long uint32_t ;
 typedef unsigned long long uint64_t ;





 typedef signed char int_least8_t ;
 typedef signed short int_least16_t ;
 typedef signed long int_least32_t ;
 typedef signed long long int_least64_t ;









 typedef unsigned char uint_least8_t ;
 typedef unsigned short uint_least16_t ;
 typedef unsigned long uint_least32_t ;
 typedef unsigned long long uint_least64_t ;





 typedef signed int int_fast8_t ;
 typedef signed int int_fast16_t ;
 typedef signed long int_fast32_t ;
 typedef signed long long int_fast64_t ;









 typedef unsigned int uint_fast8_t ;
 typedef unsigned int uint_fast16_t ;
 typedef unsigned long uint_fast32_t ;
 typedef unsigned long long uint_fast64_t ;






 typedef long intptr_t ;



 typedef unsigned long uintptr_t ;


 typedef long long intmax_t ;



 typedef unsigned long long uintmax_t ;
#line 4 "stdbool.h"
 typedef unsigned char bool ;
#line 6 "stdlib.h"
 typedef unsigned long size_t ;




 typedef char wchar_t ;













 void exit ( int ) ;

 void _Exit ( int ) ;

 void * malloc ( size_t ) ;
 void * calloc ( size_t , size_t ) ;
 void * realloc ( void * , size_t ) ;
 void free ( void * ) ;
 int system ( const char * ) ;
 int rand ( void ) ;
 void srand ( unsigned int ) ;
 double atof ( const char * ) ;
 int atoi ( const char * ) ;
 long atol ( const char * ) ;

 long long atoll ( const char * ) ;

 double strtod ( const char * , char * * ) ;
 long strtol ( const char * , char * * , int ) ;

 signed long long strtoll ( const char * , char * * , int ) ;

 unsigned long strtoul ( const char * , char * * , int ) ;

 unsigned long long strtoull ( const char * , char * * , int ) ;

 void abort ( void ) ;
 int atexit ( void ( * ) ( void ) ) ;
 char * getenv ( const char * ) ;
 void * bsearch ( const void * , const void * , size_t , size_t , int ( * ) ( const void * , const void * ) ) ;
 void qsort ( void * , size_t , size_t , int ( * ) ( const void * , const void * ) ) ;

 typedef struct {
 int quot , rem ;
 } div_t ;

 typedef struct {
 long quot , rem ;
 } ldiv_t ;


 typedef struct {
 long long quot , rem ;
 } lldiv_t ;


 div_t div ( int , int ) ;
 ldiv_t ldiv ( long , long ) ;

 lldiv_t lldiv ( long long , long long ) ;


 int abs ( int ) ;
 long labs ( long ) ;

 long long llabs ( long long ) ;



 __vattr ( "regused(" "d0" ")" ) int __asm_abs ( __reg ( "d0" ) int ) =
 "\tinline\n"
 "\ttst.l\td0\n"
 "\tbpl\t.skip\n"
 "\tneg.l\td0\n"
 ".skip\n"
 "\teinline" ;
 __vattr ( "regused(" "d0" ")" ) long __asm_labs ( __reg ( "d0" ) long ) =
 "\tinline\n"
 "\ttst.l\td0\n"
 "\tbpl\t.skip\n"
 "\tneg.l\td0\n"
 ".skip\n"
 "\teinline" ;



 __vattr ( "regused(" "d0/d1" ")" ) long long __asm_llabs ( __reg ( "d0/d1" ) long long ) =
 "\tinline\n"
 "\ttst.l\td0\n"
 "\tbpl\t.skip\n"
 "\tneg.l\td1\n"
 "\tnegx.l\td0\n"
 ".skip\n"
 "\teinline" ;





















 extern size_t _nalloc ;

 struct __exitfuncs {
 struct __exitfuncs * next ;
 void ( * func ) ( void ) ;
 } ;
#line 35 "stdio.h"
 typedef struct _iobuf
 {
 char * filehandle ;
 char * pointer ;
 char * base ;
 struct _iobuf * next ;
 struct _iobuf * prev ;
 int count ;
 int flags ;
 int bufsize ;
 } FILE ;


 extern FILE * stdin , * stdout , * stderr ;

 int _fillbuf ( FILE * ) , _putbuf ( int , FILE * ) , _flushbuf ( FILE * ) ;
 void _ttyflush ( void ) ;














 typedef long fpos_t ;
#line 4 "stdarg.h"
 typedef unsigned char * va_list ;
#line 76 "stdio.h"
 FILE * fopen ( const char * , const char * ) ;
 FILE * freopen ( const char * , const char * , FILE * ) ;
 int fflush ( FILE * ) ;
 int fclose ( FILE * ) ;
 int rename ( const char * , const char * ) ;
 int remove ( const char * ) ;
 FILE * tmpfile ( void ) ;
 char * tmpnam ( char * ) ;
 int setvbuf ( FILE * , char * , int , size_t ) ;
 void setbuf ( FILE * , char * ) ;
 int fprintf ( FILE * , const char * , ... ) ;
 int printf ( const char * , ... ) ;
 int sprintf ( char * , const char * , ... ) ;
 int snprintf ( char * , size_t , const char * , ... ) ;
 int __v0fprintf ( FILE * , const char * ) ;
 int __v0printf ( const char * ) ;
 int __v0sprintf ( char * , const char * ) ;
 int vprintf ( const char * , va_list ) ;
 int vfprintf ( FILE * , const char * , va_list ) ;
 int vsprintf ( char * , const char * , va_list ) ;
 int vsnprintf ( char * , size_t , const char * , va_list ) ;
 int fscanf ( FILE * , const char * , ... ) ;
 int scanf ( const char * , ... ) ;
 int sscanf ( const char * , const char * , ... ) ;
 int vscanf ( const char * , va_list ) ;
 int vfscanf ( FILE * , const char * , va_list ) ;
 int vsscanf ( const char * , const char * , va_list ) ;
 char * fgets ( char * , int , FILE * ) ;
 int fputs ( const char * , FILE * ) ;
 char * gets ( char * ) ;
 int puts ( const char * ) ;
 int ungetc ( int , FILE * ) ;
 size_t fread ( void * , size_t , size_t , FILE * ) ;
 size_t fwrite ( void * , size_t , size_t , FILE * ) ;
 int fseek ( FILE * , long , int ) ;
 void rewind ( FILE * ) ;
 long ftell ( FILE * ) ;
 int fgetpos ( FILE * , fpos_t * ) ;
 int fsetpos ( FILE * , const fpos_t * ) ;
 void perror ( const char * ) ;
 int fgetc ( FILE * ) ;
 int fputc ( int , FILE * ) ;
 int getchar ( void ) ;
 int putchar ( int ) ;


















#pragma printflike printf
#pragma printflike fprintf
#pragma printflike sprintf
#pragma printflike snprintf
#pragma scanflike scanf
#pragma scanflike fscanf
#pragma scanflike sscanf
#line 12 "string.h"
 void * memcpy ( void * , const void * , size_t ) ;
 void * memmove ( void * , const void * , size_t ) ;
 char * strcpy ( char * , const char * ) ;
 char * strncpy ( char * , const char * , size_t ) ;
 char * strcat ( char * , const char * ) ;
 char * strncat ( char * , const char * , size_t ) ;
 int memcmp ( const void * , const void * , size_t ) ;
 int strcmp ( const char * , const char * ) ;
 int strncmp ( const char * , const char * , size_t ) ;
 void * memchr ( const void * , int , size_t ) ;
 char * strchr ( const char * , int ) ;
 size_t strcspn ( const char * , const char * ) ;
 char * strpbrk ( const char * , const char * ) ;
 char * strrchr ( const char * , int ) ;
 size_t strspn ( const char * , const char * ) ;
 char * strstr ( const char * , const char * ) ;
 void * memset ( void * , int , size_t ) ;
 size_t strlen ( const char * ) ;
 char * strtok ( char * , const char * ) ;
 char * strerror ( int ) ;
 int strcoll ( const char * , const char * ) ;
 size_t strxfrm ( char * , const char * , size_t ) ;



 void * __asm_memcpy ( __reg ( "a0" ) void * , __reg ( "a1" ) const void * ,
 __reg ( "d2" ) size_t ) =
 "\tinline\n"
 "\tmove.l\ta0,d0\n"
 "\tcmp.l\t#16,d2\n"
 "\tblo\t.l5\n"
 "\tmoveq\t#1,d1\n"
 "\tand.b\td0,d1\n"
 "\tbeq\t.l1\n"
 "\tmove.b\t(a1)+,(a0)+\n"
 "\tsubq.l\t#1,d2\n"
 ".l1\n"
 "\tmove.l\ta1,d1\n"
 "\tand.b\t#1,d1\n"
 "\tbeq\t.l3\n"
 "\tcmp.l\t#$10000,d2\n"
 "\tblo\t.l5\n"
 ".l2\n"
 "\tmove.b\t(a1)+,(a0)+\n"
 "\tsubq.l\t#1,d2\n"
 "\tbne\t.l2\n"
 "\tbra\t.l7\n"
 ".l3\n"
 "\tmoveq\t#3,d1\n"
 "\tand.l\td2,d1\n"
 "\tsub.l\td1,d2\n"
 ".l4\n"
 "\tmove.l\t(a1)+,(a0)+\n"
 "\tsubq.l\t#4,d2\n"
 "\tbne\t.l4\n"
 "\tmove.w\td1,d2\n"
 ".l5\n"
 "\tsubq.w\t#1,d2\n"
 "\tblo\t.l7\n"
 ".l6\n"
 "\tmove.b\t(a1)+,(a0)+\n"
 "\tdbf\td2,.l6\n"
 ".l7\n"
 "\teinline" ;
 void * __asm_memcpy_desc ( __reg ( "a0" ) void * , __reg ( "a1" ) const void * ,
 __reg ( "d2" ) size_t ) =
 "\tinline\n"
 "\tcmp.l\t#16,d2\n"
 "\tblo\t.l5\n"
 "\tmoveq\t#1,d1\n"
 "\tmove.l\ta0,d0\n"
 "\tand.b\td1,d0\n"
 "\tbeq\t.l1\n"
 "\tmove.b\t-(a1),-(a0)\n"
 "\tsubq.l\t#1,d2\n"
 ".l1\n"
 "\tmove.l\ta1,d0\n"
 "\tand.b\td1,d0\n"
 "\tbeq\t.l3\n"
 "\tcmp.l\t#$10000,d2\n"
 "\tblo\t.l5\n"
 ".l2\n"
 "\tmove.b\t-(a1),-(a0)\n"
 "\tsubq.l\t#1,d2\n"
 "\tbne\t.l2\n"
 "\tbra\t.l7\n"
 ".l3\n"
 "\tmoveq\t#3,d1\n"
 "\tand.l\td2,d1\n"
 "\tsub.l\td1,d2\n"
 ".l4\n"
 "\tmove.l\t-(a1),-(a0)\n"
 "\tsubq.l\t#4,d2\n"
 "\tbne\t.l4\n"
 "\tmove.w\td1,d2\n"
 ".l5\n"
 "\tsubq.w\t#1,d2\n"
 "\tblo\t.l7\n"
 ".l6\n"
 "\tmove.b\t-(a1),-(a0)\n"
 "\tdbf\td2,.l6\n"
 ".l7\n"
 "\tmove.l\ta0,d0\n"
 "\teinline" ;
 void * __asm_memset ( __reg ( "a0" ) void * , __reg ( "d0" ) int , __reg ( "d2" ) size_t ) =
 "\tinline\n"
 "\tmove.l\ta0,a1\n"
 "\tcmp.l\t#16,d2\n"
 "\tblo\t.l3\n"
 "\tmove.l\ta0,d1\n"
 "\tand.b\t#1,d1\n"
 "\tbeq\t.l1\n"
 "\tmove.b\td0,(a0)+\n"
 "\tsubq.l\t#1,d2\n"
 ".l1\n"
 "\tmove.b\td0,d1\n"
 "\tlsl.w\t#8,d0\n"
 "\tmove.b\td1,d0\n"
 "\tmove.w\td0,d1\n"
 "\tswap\td0\n"
 "\tmove.w\td1,d0\n"
 "\tmoveq\t#3,d1\n"
 "\tand.l\td2,d1\n"
 "\tsub.l\td1,d2\n"
 ".l2\n"
 "\tmove.l\td0,(a0)+\n"
 "\tsubq.l\t#4,d2\n"
 "\tbne\t.l2\n"
 "\tmove.w\td1,d2\n"
 ".l3\n"
 "\tsubq.w\t#1,d2\n"
 "\tblo\t.l5\n"
 ".l4\n"
 "\tmove.b\td0,(a0)+\n"
 "\tdbf\td2,.l4\n"
 ".l5\n"
 "\tmove.l\ta1,d0\n"
 "\teinline" ;









































































































 __vattr ( "regused(" "d0/a0" ")" ) size_t __asm_strlen ( __reg ( "a0" ) const char * ) =
 "\tinline\n"
 "\tmove.l\ta0,d0\n"
 ".l1\n"
 "\ttst.b\t(a0)+\n"
 "\tbne\t.l1\n"
 "\tsub.l\ta0,d0\n"
 "\tnot.l\td0\n"
 "\teinline" ;
 __vattr ( "regused(" "d0/a0/a1" ")" ) char * __asm_strcpy ( __reg ( "a0" ) char * , __reg ( "a1" ) const char * ) =
 "\tinline\n"
 "\tmove.l\ta0,d0\n"
 ".l1\n"
 "\tmove.b\t(a1)+,(a0)+\n"
 "\tbne\t.l1\n"
 "\teinline" ;
 char * __asm_strncpy ( __reg ( "a0" ) char * , __reg ( "a1" ) const char * ,
 __reg ( "d1" ) size_t ) =
 "\tinline\n"
 "\tmove.l\ta0,d0\n"
 "\tbra\t.l2\n"
 ".l1\n"
 "\tmove.b\t(a1),(a0)+\n"
 "\tbeq\t.l2\n"
 "\taddq.l\t#1,a1\n"
 ".l2\n"
 "\tsubq.l\t#1,d1\n"
 "\tbpl\t.l1\n"
 "\teinline" ;
 int __asm_strcmp ( __reg ( "a0" ) const char * , __reg ( "a1" ) const char * ) =
 "\tinline\n"
 "\tmoveq\t#0,d0\n"
 "\tmoveq\t#0,d1\n"
 ".l1\n"
 "\tmove.b\t(a0)+,d0\n"
 "\tmove.b\t(a1)+,d1\n"
 "\tbeq\t.l2\n"
 "\tsub.l\td1,d0\n"
 "\tbeq\t.l1\n"
 "\tmoveq\t#0,d1\n"
 ".l2\n"
 "\tsub.l\td1,d0\n"
 "\teinline" ;
 int __asm_strncmp ( __reg ( "a0" ) const char * , __reg ( "a1" ) const char * ,
 __reg ( "d2" ) size_t ) =
 "\tinline\n"
 "\tmoveq\t#0,d0\n"
 "\tmoveq\t#0,d1\n"
 ".l1\n"
 "\tsubq.l\t#1,d2\n"
 "\tbmi\t.l3\n"
 "\tmove.b\t(a0)+,d0\n"
 "\tmove.b\t(a1)+,d1\n"
 "\tbeq\t.l2\n"
 "\tsub.l\td1,d0\n"
 "\tbeq\t.l1\n"
 "\tmoveq\t#0,d1\n"
 ".l2\n"
 "\tsub.l\td1,d0\n"
 ".l3\n"
 "\teinline" ;
 __vattr ( "regused(" "d0/a0/a1" ")" ) char * __asm_strcat ( __reg ( "a0" ) char * , __reg ( "a1" ) const char * ) =
 "\tinline\n"
 "\tmove.l\ta0,d0\n"
 ".l1\n"
 "\ttst.b\t(a0)+\n"
 "\tbne\t.l1\n"
 "\tsubq.l\t#1,a0\n"
 ".l2\n"
 "\tmove.b\t(a1)+,(a0)+\n"
 "\tbne\t.l2\n"
 "\teinline" ;
 char * __asm_strncat ( __reg ( "a0" ) char * , __reg ( "a1" ) const char * ,
 __reg ( "d1" ) size_t ) =
 "\tinline\n"
 "\tmove.l\ta0,d0\n"
 "\ttst.l\td1\n"
 "\tbeq\t.l4\n"
 ".l1\n"
 "\ttst.b\t(a0)+\n"
 "\tbne\t.l1\n"
 "\tsubq.l\t#1,a0\n"
 ".l2\n"
 "\tmove.b\t(a1)+,(a0)+\n"
 "\tbeq\t.l3\n"
 "\tsubq.l\t#1,d1\n"
 "\tbne\t.l2\n"
 ".l3\n"
 "\tclr.b\t(a0)\n"
 ".l4\n"
 "\teinline" ;
 __vattr ( "regused(" "d0/d1/a0" ")" ) char * __asm_strrchr ( __reg ( "a0" ) const char * , __reg ( "d1" ) int ) =
 "\tinline\n"
 "\tmoveq\t#0,d0\n"
 ".l1\n"
 "\tcmp.b\t(a0),d1\n"
 "\tbne\t.l2\n"
 "\tmove.l\ta0,d0\n"
 ".l2\n"
 "\ttst.b\t(a0)+\n"
 "\tbne\t.l1\n"
 "\teinline" ;
#line 31 "platform\platform.h"
 void * whd_malloc_debug ( size_t size , const char * file , int line ) ;
 void whd_free_debug ( void * ptr , const char * file , int line ) ;
 char * whd_strdup_debug ( const char * str , const char * file , int line ) ;
 void whd_memory_report ( void ) ;
 void whd_memory_init ( void ) ;
 void whd_memory_suspend_logging ( void ) ;
 void whd_memory_resume_logging ( void ) ;
#line 76 "exec\types.h"
 typedef void * APTR ;




 typedef const void * CONST_APTR ;































 typedef int32_t LONG ;
 typedef uint32_t ULONG ;
 typedef uint32_t LONGBITS ;
 typedef int16_t WORD ;
 typedef uint16_t UWORD ;
 typedef uint16_t WORDBITS ;
 typedef int8_t BYTE ;
 typedef uint8_t UBYTE ;
 typedef uint8_t BYTEBITS ;
 typedef uint16_t RPTR ;




 typedef int16_t SHORT ;
 typedef uint16_t USHORT ;
 typedef int16_t COUNT ;
 typedef uint16_t UCOUNT ;
 typedef uint32_t CPTR ;






 typedef unsigned char * STRPTR ;








 typedef const unsigned char * CONST_STRPTR ;



 typedef float FLOAT ;
 typedef double DOUBLE ;




 typedef unsigned char TEXT ;











 typedef int16_t BOOL ;
#line 22 "exec\nodes.h"
 struct Node {
 struct Node * ln_Succ ;
 struct Node * ln_Pred ;
 UBYTE ln_Type ;
 BYTE ln_Pri ;
 char * ln_Name ;
 } ;


 struct MinNode {
 struct MinNode * mln_Succ ;
 struct MinNode * mln_Pred ;
 } ;
#line 20 "exec\lists.h"
 struct List {
 struct Node * lh_Head ;
 struct Node * lh_Tail ;
 struct Node * lh_TailPred ;
 UBYTE lh_Type ;
 UBYTE l_pad ;
 } ;




 struct MinList {
 struct MinNode * mlh_Head ;
 struct MinNode * mlh_Tail ;
 struct MinNode * mlh_TailPred ;
 } ;
#line 27 "listview_columns_api.h"
 typedef enum {
 ITIDY_MODE_FULL ,
 ITIDY_MODE_FULL_NO_SORT ,
 ITIDY_MODE_SIMPLE ,
 ITIDY_MODE_SIMPLE_PAGINATED
 } iTidy_ListViewMode ;





 typedef enum {
 ITIDY_ALIGN_LEFT ,
 ITIDY_ALIGN_RIGHT ,
 ITIDY_ALIGN_CENTER
 } iTidy_ColumnAlign ;





 typedef enum {
 ITIDY_SORT_NONE ,
 ITIDY_SORT_ASCENDING ,
 ITIDY_SORT_DESCENDING
 } iTidy_SortOrder ;

 typedef enum {
 ITIDY_COLTYPE_TEXT ,
 ITIDY_COLTYPE_NUMBER ,
 ITIDY_COLTYPE_DATE
 } iTidy_ColumnType ;





 typedef struct {
 const char * title ;
 int min_width ;
 int max_width ;
 iTidy_ColumnAlign align ;
 BOOL flexible ;
 BOOL is_path ;







 iTidy_SortOrder default_sort ;
 iTidy_ColumnType sort_type ;
 } iTidy_ColumnConfig ;











 typedef enum {
 ITIDY_ROW_DATA = 0 ,
 ITIDY_ROW_NAV_PREV = 1 ,
 ITIDY_ROW_NAV_NEXT = 2
 } iTidy_RowType ;























 typedef struct {
 struct Node node ;
 const char * * display_data ;
 const char * * sort_keys ;
 int num_columns ;
 iTidy_RowType row_type ;
 } iTidy_ListViewEntry ;











 typedef struct {
 int column_index ;
 int char_start ;
 int char_end ;
 int char_width ;
 int pixel_start ;
 int pixel_end ;
 iTidy_SortOrder sort_state ;
 iTidy_ColumnType column_type ;
 } iTidy_ColumnState ;










 typedef struct {
 iTidy_ColumnState * columns ;
 int num_columns ;
 int separator_width ;


 int current_page ;
 int total_pages ;
 int last_nav_direction ;
 int auto_select_row ;
 BOOL sorting_disabled ;
 } iTidy_ListViewState ;





 typedef struct {
 iTidy_ColumnConfig * columns ;
 int num_columns ;
 struct List * entries ;
 int total_char_width ;
 iTidy_ListViewState * * out_state ;
 iTidy_ListViewMode mode ;
 int page_size ;
 int current_page ;
 int * out_total_pages ;
 int nav_direction ;
 } iTidy_ListViewOptions ;





 typedef struct iTidy_ListViewSession {
 iTidy_ListViewOptions options ;
 struct List * display_list ;
 struct List * entry_list ;
 iTidy_ListViewState * state ;
 iTidy_ListViewState * * state_target ;
 BOOL owns_entry_list ;
 BOOL formatted_once ;
 } iTidy_ListViewSession ;


































































 void iTidy_InitListViewOptions ( iTidy_ListViewOptions * options ) ;
 BOOL iTidy_ValidateListViewOptions ( const iTidy_ListViewOptions * options , const char * * out_error_text ) ;
 struct List * iTidy_FormatListViewColumnsEx ( const iTidy_ListViewOptions * options ) ;
 iTidy_ListViewSession * iTidy_ListViewSessionCreate ( const iTidy_ListViewOptions * options ) ;
 struct List * iTidy_ListViewSessionFormat ( iTidy_ListViewSession * session ) ;
 void iTidy_ListViewSessionDestroy ( iTidy_ListViewSession * session ) ;

 struct List * iTidy_FormatListViewColumns (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 struct List * entries ,
 int total_char_width ,
 iTidy_ListViewState * * out_state ,
 iTidy_ListViewMode mode ,
 int page_size ,
 int current_page ,
 int * out_total_pages ,
 int nav_direction
 ) ;

























































 void iTidy_SortListViewEntries ( struct List * list , int col , iTidy_ColumnType type , BOOL ascending ) ;

 BOOL iTidy_ResortListViewByClick (
 struct List * formatted_list ,
 struct List * entry_list ,
 iTidy_ListViewState * state ,
 int mouse_x ,
 int mouse_y ,
 int header_top ,
 int header_height ,
 int gadget_left ,
 int font_width ,
 iTidy_ColumnConfig * columns
 ) ;


















































 BOOL iTidy_HandleListViewSort (
 struct Window * window ,
 struct Gadget * listview_gadget ,
 struct List * formatted_list ,
 struct List * entry_list ,
 iTidy_ListViewState * state ,
 int mouse_x ,
 int mouse_y ,
 int font_height ,
 int font_width ,
 iTidy_ColumnConfig * columns ,
 int num_columns
 ) ;



































 iTidy_ListViewEntry * iTidy_GetSelectedEntry ( struct List * entry_list , LONG listview_row ) ;












 typedef struct {
 iTidy_ListViewEntry * entry ;
 int column ;
 const char * display_value ;
 const char * sort_key ;
 iTidy_ColumnType column_type ;
 } iTidy_ListViewClick ;








 typedef enum {
 ITIDY_LV_EVENT_NONE ,
 ITIDY_LV_EVENT_HEADER_SORTED ,
 ITIDY_LV_EVENT_ROW_CLICK ,
 ITIDY_LV_EVENT_NAV_HANDLED
 } iTidy_ListViewEventType ;








 typedef struct {
 iTidy_ListViewEventType type ;


 BOOL did_sort ;


 int sorted_column ;
 iTidy_SortOrder sort_order ;


 iTidy_ListViewEntry * entry ;
 int column ;
 const char * display_value ;
 const char * sort_key ;
 iTidy_ColumnType column_type ;


 int nav_direction ;
 } iTidy_ListViewEvent ;








































































 BOOL iTidy_HandleListViewGadgetUp (
 struct Window * window ,
 struct Gadget * gadget ,
 WORD mouse_x ,
 WORD mouse_y ,
 struct List * entry_list ,
 struct List * display_list ,
 iTidy_ListViewState * state ,
 int font_height ,
 int font_width ,
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 iTidy_ListViewEvent * out_event
 ) ;
























 int iTidy_GetClickedColumn ( iTidy_ListViewState * state , WORD mouse_x , WORD gadget_left ) ;



















































 iTidy_ListViewClick iTidy_GetListViewClick (
 struct List * entry_list ,
 iTidy_ListViewState * state ,
 LONG listview_row ,
 WORD mouse_x ,
 WORD gadget_left
 ) ;















 void iTidy_FreeFormattedList ( struct List * list ) ;








 void iTidy_FreeListViewState ( iTidy_ListViewState * state ) ;












































 void itidy_free_listview_entries ( struct List * entry_list ,
 struct List * display_list ,
 iTidy_ListViewState * state ) ;





















 struct List * iTidy_FormatListViewColumns_Legacy (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 const char * * * data_rows ,
 int num_rows ,
 int total_char_width
 ) ;


















 BOOL iTidy_CalculateColumnWidths (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 struct List * entries ,
 int total_char_width ,
 int * out_widths
 ) ;
#line 8 "proto\dos.h"
#pragma stdargs-on
#line 24 "exec\tasks.h"
 struct Task {
 struct Node tc_Node ;
 UBYTE tc_Flags ;
 UBYTE tc_State ;
 BYTE tc_IDNestCnt ;
 BYTE tc_TDNestCnt ;
 ULONG tc_SigAlloc ;
 ULONG tc_SigWait ;
 ULONG tc_SigRecvd ;
 ULONG tc_SigExcept ;
 UWORD tc_TrapAlloc ;
 UWORD tc_TrapAble ;
 APTR tc_ExceptData ;
 APTR tc_ExceptCode ;
 APTR tc_TrapData ;
 APTR tc_TrapCode ;
 APTR tc_SPReg ;
 APTR tc_SPLower ;
 APTR tc_SPUpper ;
 void ( * tc_Switch ) ( ) ;
 void ( * tc_Launch ) ( ) ;
 struct List tc_MemEntry ;
 APTR tc_UserData ;
 } ;




 struct StackSwapStruct {
 APTR stk_Lower ;
 ULONG stk_Upper ;
 APTR stk_Pointer ;
 } ;
#line 27 "exec\ports.h"
 struct MsgPort {
 struct Node mp_Node ;
 UBYTE mp_Flags ;
 UBYTE mp_SigBit ;
 void * mp_SigTask ;
 struct List mp_MsgList ;
 } ;












 struct Message {
 struct Node mn_Node ;
 struct MsgPort * mn_ReplyPort ;
 UWORD mn_Length ;


 } ;
#line 33 "exec\libraries.h"
 struct Library {
 struct Node lib_Node ;
 UBYTE lib_Flags ;
 UBYTE lib_pad ;
 UWORD lib_NegSize ;
 UWORD lib_PosSize ;
 UWORD lib_Version ;
 UWORD lib_Revision ;
 APTR lib_IdString ;
 ULONG lib_Sum ;
 UWORD lib_OpenCnt ;
 } ;
#line 33 "exec\semaphores.h"
 struct SemaphoreRequest
 {
 struct MinNode sr_Link ;
 struct Task * sr_Waiter ;
 } ;


 struct SignalSemaphore
 {
 struct Node ss_Link ;
 WORD ss_NestCount ;
 struct MinList ss_WaitQueue ;
 struct SemaphoreRequest ss_MultipleLink ;
 struct Task * ss_Owner ;
 WORD ss_QueueCount ;
 } ;


 struct SemaphoreMessage
 {
 struct Message ssm_Message ;
 struct SignalSemaphore * ssm_Semaphore ;
 } ;






 struct Semaphore
 {
 struct MsgPort sm_MsgPort ;
 WORD sm_Bids ;
 } ;
#line 17 "exec\io.h"
 struct IORequest {
 struct Message io_Message ;
 struct Device * io_Device ;
 struct Unit * io_Unit ;
 UWORD io_Command ;
 UBYTE io_Flags ;
 BYTE io_Error ;
 } ;

 struct IOStdReq {
 struct Message io_Message ;
 struct Device * io_Device ;
 struct Unit * io_Unit ;
 UWORD io_Command ;
 UBYTE io_Flags ;
 BYTE io_Error ;
 ULONG io_Actual ;
 ULONG io_Length ;
 APTR io_Data ;
 ULONG io_Offset ;
 } ;
#line 32 "devices\timer.h"
 struct TimeVal {
 ULONG tv_secs ;
 ULONG tv_micro ;
 } ;

 struct TimeRequest {
 struct IORequest tr_node ;
 struct TimeVal tr_time ;
 } ;

































 struct timeval {
 ULONG tv_secs ;
 ULONG tv_micro ;
 } ;

 struct timerequest {
 struct IORequest tr_node ;
 struct timeval tr_time ;
 } ;

 typedef struct timeval TimeVal_Type ;
 typedef struct timerequest TimeRequest_Type ;















 struct EClockVal {
 ULONG ev_hi ;
 ULONG ev_lo ;
 } ;
#line 52 "dos\dos.h"
 struct DateStamp {
 LONG ds_Days ;
 LONG ds_Minute ;
 LONG ds_Tick ;
 } ;




 struct FileInfoBlock {
 LONG fib_DiskKey ;
 LONG fib_DirEntryType ;

 TEXT fib_FileName [ 108 ] ;
 LONG fib_Protection ;
 LONG fib_EntryType ;
 LONG fib_Size ;
 LONG fib_NumBlocks ;
 struct DateStamp fib_Date ;
 TEXT fib_Comment [ 80 ] ;





 UWORD fib_OwnerUID ;
 UWORD fib_OwnerGID ;

 UBYTE fib_Reserved [ 32 ] ;
 } ;


















































 typedef long BPTR ;
 typedef long BSTR ;















 struct InfoData {
 LONG id_NumSoftErrors ;
 LONG id_UnitNumber ;
 LONG id_DiskState ;
 LONG id_NumBlocks ;
 LONG id_NumBlocksUsed ;
 LONG id_BytesPerBlock ;
 LONG id_DiskType ;
 BPTR id_VolumeNode ;
 LONG id_InUse ;
 } ;
#line 40 "dos\dosextens.h"
 struct Process {
 struct Task pr_Task ;
 struct MsgPort pr_MsgPort ;
 WORD pr_Pad ;
 BPTR pr_SegList ;
 LONG pr_StackSize ;
 APTR pr_GlobVec ;
 LONG pr_TaskNum ;
 BPTR pr_StackBase ;
 LONG pr_Result2 ;
 BPTR pr_CurrentDir ;
 BPTR pr_CIS ;
 BPTR pr_COS ;
 APTR pr_ConsoleTask ;
 APTR pr_FileSystemTask ;
 BPTR pr_CLI ;
 APTR pr_ReturnAddr ;
 APTR pr_PktWait ;
 APTR pr_WindowPtr ;


 BPTR pr_HomeDir ;
 LONG pr_Flags ;
 void ( * pr_ExitCode ) ( ) ;
 LONG pr_ExitData ;
 STRPTR pr_Arguments ;
 struct MinList pr_LocalVars ;
 ULONG pr_ShellPrivate ;
 BPTR pr_CES ;
 } ;
























 struct FileHandle {
 struct Message * fh_Link ;
 struct MsgPort * fh_Port ;

 struct MsgPort * fh_Type ;

 BPTR fh_Buf ;
 LONG fh_Pos ;
 LONG fh_End ;
 LONG fh_Funcs ;

 LONG fh_Func2 ;
 LONG fh_Func3 ;
 LONG fh_Args ;

 LONG fh_Arg2 ;
 } ;



 struct DosPacket {
 struct Message * dp_Link ;
 struct MsgPort * dp_Port ;

 LONG dp_Type ;


 LONG dp_Res1 ;



 LONG dp_Res2 ;






 LONG dp_Arg1 ;
 LONG dp_Arg2 ;
 LONG dp_Arg3 ;
 LONG dp_Arg4 ;
 LONG dp_Arg5 ;
 LONG dp_Arg6 ;
 LONG dp_Arg7 ;
 } ;





 struct StandardPacket {
 struct Message sp_Msg ;
 struct DosPacket sp_Pkt ;
 } ;




















































































 struct ErrorString {
 LONG * estr_Nums ;
 STRPTR estr_Strings ;
 } ;






 struct DosLibrary {
 struct Library dl_lib ;
 struct RootNode * dl_Root ;
 APTR dl_GV ;
 LONG dl_A2 ;
 LONG dl_A5 ;
 LONG dl_A6 ;
 struct ErrorString * dl_Errors ;
 struct timerequest * dl_TimeReq ;
 struct Library * dl_UtilityBase ;
 struct Library * dl_IntuitionBase ;
 } ;



 struct RootNode {
 BPTR rn_TaskArray ;


 BPTR rn_ConsoleSegment ;
 struct DateStamp rn_Time ;
 LONG rn_RestartSeg ;
 BPTR rn_Info ;
 BPTR rn_FileHandlerSegment ;
 struct MinList rn_CliList ;

 struct MsgPort * rn_BootProc ;
 BPTR rn_ShellSegment ;
 LONG rn_Flags ;
 } ;







 struct CliProcList {
 struct MinNode cpl_Node ;
 LONG cpl_First ;
 struct MsgPort * * cpl_Array ;




 } ;

 struct DosInfo {
 BPTR di_McName ;

 BPTR di_DevInfo ;
 BPTR di_Devices ;
 BPTR di_Handlers ;
 APTR di_NetHand ;
 struct SignalSemaphore di_DevLock ;
 struct SignalSemaphore di_EntryLock ;
 struct SignalSemaphore di_DeleteLock ;
 } ;




 struct Segment {
 BPTR seg_Next ;
 LONG seg_UC ;
 BPTR seg_Seg ;
 UBYTE seg_Name [ 4 ] ;
 } ;









 struct CommandLineInterface {
 LONG cli_Result2 ;
 BSTR cli_SetName ;
 BPTR cli_CommandDir ;
 LONG cli_ReturnCode ;
 BSTR cli_CommandName ;
 LONG cli_FailLevel ;
 BSTR cli_Prompt ;
 BPTR cli_StandardInput ;
 BPTR cli_CurrentInput ;
 BSTR cli_CommandFile ;
 LONG cli_Interactive ;
 LONG cli_Background ;
 BPTR cli_CurrentOutput ;
 LONG cli_DefaultStack ;
 BPTR cli_StandardOutput ;
 BPTR cli_Module ;
 } ;










 struct DeviceList {
 BPTR dl_Next ;
 LONG dl_Type ;
 struct MsgPort * dl_Task ;
 BPTR dl_Lock ;
 struct DateStamp dl_VolumeDate ;
 BPTR dl_LockList ;
 LONG dl_DiskType ;
 LONG dl_unused ;
 BSTR dl_Name ;
 } ;



 struct DevInfo {
 BPTR dvi_Next ;
 LONG dvi_Type ;
 APTR dvi_Task ;
 BPTR dvi_Lock ;
 BSTR dvi_Handler ;
 LONG dvi_StackSize ;
 LONG dvi_Priority ;
 LONG dvi_Startup ;
 BPTR dvi_SegList ;
 BPTR dvi_GlobVec ;
 BSTR dvi_Name ;
 } ;



 struct DosList {
 BPTR dol_Next ;
 LONG dol_Type ;
 struct MsgPort * dol_Task ;
 BPTR dol_Lock ;
 union {
 struct {
 BSTR dol_Handler ;
 LONG dol_StackSize ;
 LONG dol_Priority ;
 ULONG dol_Startup ;
 BPTR dol_SegList ;
 BPTR dol_GlobVec ;


 } dol_handler ;

 struct {
 struct DateStamp dol_VolumeDate ;
 BPTR dol_LockList ;
 LONG dol_DiskType ;
 } dol_volume ;

 struct {
 STRPTR dol_AssignName ;
 struct AssignList * dol_List ;
 } dol_assign ;

 } dol_misc ;

 BSTR dol_Name ;
 } ;



 struct AssignList {
 struct AssignList * al_Next ;
 BPTR al_Lock ;
 } ;










 struct DevProc {
 struct MsgPort * dvp_Port ;
 BPTR dvp_Lock ;
 ULONG dvp_Flags ;
 struct DosList * dvp_DevNode ;
 } ;





























 struct FileLock {
 BPTR fl_Link ;
 LONG fl_Key ;
 LONG fl_Access ;
 struct MsgPort * fl_Task ;
 BPTR fl_Volume ;
 } ;
#line 26 "dos\record.h"
 struct RecordLock {
 BPTR rec_FH ;
 ULONG rec_Offset ;
 ULONG rec_Length ;
 ULONG rec_Mode ;
 } ;
#line 55 "dos\rdargs.h"
 struct CSource {
 STRPTR CS_Buffer ;
 LONG CS_Length ;
 LONG CS_CurChr ;
 } ;































 struct RDArgs {
 struct CSource RDA_Source ;
 LONG RDA_DAList ;
 STRPTR RDA_Buffer ;
 LONG RDA_BufSiz ;
 STRPTR RDA_ExtHelp ;
 LONG RDA_Flags ;
 } ;
#line 49 "dos\dosasl.h"
 struct AnchorPath {
 struct AChain * ap_Base ;

 struct AChain * ap_Last ;

 LONG ap_BreakBits ;
 LONG ap_FoundBreak ;
 BYTE ap_Flags ;
 BYTE ap_Reserved ;
 WORD ap_Strlen ;

 struct FileInfoBlock ap_Info ;
 TEXT ap_Buf [ 1 ] ;

 } ;
































 struct AChain {
 struct AChain * an_Child ;
 struct AChain * an_Parent ;
 BPTR an_Lock ;
 struct FileInfoBlock an_Info ;
 BYTE an_Flags ;
 TEXT an_String [ 1 ] ;
 } ;
#line 21 "dos\var.h"
 struct LocalVar {
 struct Node lv_Node ;
 UWORD lv_Flags ;
 STRPTR lv_Value ;
 ULONG lv_Len ;
 } ;
#line 33 "dos\notify.h"
 struct NotifyMessage {
 struct Message nm_ExecMessage ;
 ULONG nm_Class ;
 UWORD nm_Code ;
 struct NotifyRequest * nm_NReq ;
 ULONG nm_DoNotTouch ;
 ULONG nm_DoNotTouch2 ;
 } ;




 struct NotifyRequest {
 STRPTR nr_Name ;
 STRPTR nr_FullName ;
 ULONG nr_UserData ;
 ULONG nr_Flags ;

 union {

 struct {
 struct MsgPort * nr_Port ;
 } nr_Msg ;

 struct {
 struct Task * nr_Task ;
 UBYTE nr_SignalNum ;
 UBYTE nr_pad [ 3 ] ;
 } nr_Signal ;
 } nr_stuff ;

 ULONG nr_Reserved [ 4 ] ;


 ULONG nr_MsgCount ;
 struct MsgPort * nr_Handler ;
 } ;
#line 23 "dos\datetime.h"
 struct DateTime
 {
 struct DateStamp dat_Stamp ;
 UBYTE dat_Format ;
 UBYTE dat_Flags ;
 STRPTR dat_StrDay ;
 STRPTR dat_StrDate ;
 STRPTR dat_StrTime ;
 } ;
#line 27 "utility\hooks.h"
 struct Hook
 {
 struct MinNode h_MinNode ;
 ULONG ( * h_Entry ) ( ) ;
 ULONG ( * h_SubEntry ) ( ) ;
 APTR h_Data ;
 } ;




 typedef unsigned long ( * HOOKFUNC ) ( ) ;
#line 38 "dos\exall.h"
 struct ExAllData {
 struct ExAllData * ed_Next ;
 STRPTR ed_Name ;
 LONG ed_Type ;
 ULONG ed_Size ;
 ULONG ed_Prot ;
 ULONG ed_Days ;
 ULONG ed_Mins ;
 ULONG ed_Ticks ;
 STRPTR ed_Comment ;
 UWORD ed_OwnerUID ;
 UWORD ed_OwnerGID ;
 } ;












 struct ExAllControl {
 ULONG eac_Entries ;
 ULONG eac_LastKey ;
 STRPTR eac_MatchString ;
 struct Hook * eac_MatchFunc ;
 } ;
#line 29 "utility\tagitem.h"
 typedef ULONG Tag ;

 struct TagItem
 {
 Tag ti_Tag ;
 ULONG ti_Data ;
 } ;
#line 44 "clib\dos_protos.h"
 BPTR Open ( CONST_STRPTR name , LONG accessMode ) ;
 LONG Close ( BPTR file ) ;
 LONG Read ( BPTR file , APTR buffer , LONG length ) ;
 LONG Write ( BPTR file , CONST_APTR buffer , LONG length ) ;
 BPTR Input ( void ) ;
 BPTR Output ( void ) ;
 LONG Seek ( BPTR file , LONG position , LONG offset ) ;
 LONG DeleteFile ( CONST_STRPTR name ) ;
 LONG Rename ( CONST_STRPTR oldName , CONST_STRPTR newName ) ;
 BPTR Lock ( CONST_STRPTR name , LONG type ) ;
 void UnLock ( BPTR lock ) ;
 BPTR DupLock ( BPTR lock ) ;
 LONG Examine ( BPTR lock , struct FileInfoBlock * fileInfoBlock ) ;
 LONG ExNext ( BPTR lock , struct FileInfoBlock * fileInfoBlock ) ;
 LONG Info ( BPTR lock , struct InfoData * parameterBlock ) ;
 BPTR CreateDir ( CONST_STRPTR name ) ;
 BPTR CurrentDir ( BPTR lock ) ;
 LONG IoErr ( void ) ;
 struct MsgPort * CreateProc ( CONST_STRPTR name , LONG pri , BPTR segList , LONG stackSize ) ;
 void Exit ( LONG returnCode ) ;
 BPTR LoadSeg ( CONST_STRPTR name ) ;
 void UnLoadSeg ( BPTR seglist ) ;
 struct MsgPort * DeviceProc ( CONST_STRPTR name ) ;
 LONG SetComment ( CONST_STRPTR name , CONST_STRPTR comment ) ;
 LONG SetProtection ( CONST_STRPTR name , LONG protect ) ;
 struct DateStamp * DateStamp ( struct DateStamp * date ) ;
 void Delay ( LONG timeout ) ;
 LONG WaitForChar ( BPTR file , LONG timeout ) ;
 BPTR ParentDir ( BPTR lock ) ;
 LONG IsInteractive ( BPTR file ) ;
 LONG Execute ( CONST_STRPTR string , BPTR file , BPTR file2 ) ;


 APTR AllocDosObject ( ULONG type , const struct TagItem * tags ) ;
 APTR AllocDosObjectTagList ( ULONG type , const struct TagItem * tags ) ;
 APTR AllocDosObjectTags ( ULONG type , ULONG tag1type , ... ) ;
 void FreeDosObject ( ULONG type , APTR ptr ) ;

 LONG DoPkt ( struct MsgPort * port , LONG action , LONG arg1 , LONG arg2 , LONG arg3 , LONG arg4 , LONG arg5 ) ;
 LONG DoPkt0 ( struct MsgPort * port , LONG action ) ;
 LONG DoPkt1 ( struct MsgPort * port , LONG action , LONG arg1 ) ;
 LONG DoPkt2 ( struct MsgPort * port , LONG action , LONG arg1 , LONG arg2 ) ;
 LONG DoPkt3 ( struct MsgPort * port , LONG action , LONG arg1 , LONG arg2 , LONG arg3 ) ;
 LONG DoPkt4 ( struct MsgPort * port , LONG action , LONG arg1 , LONG arg2 , LONG arg3 , LONG arg4 ) ;
 void SendPkt ( struct DosPacket * dp , struct MsgPort * port , struct MsgPort * replyport ) ;
 struct DosPacket * WaitPkt ( void ) ;
 void ReplyPkt ( struct DosPacket * dp , LONG res1 , LONG res2 ) ;
 void AbortPkt ( struct MsgPort * port , struct DosPacket * pkt ) ;

 BOOL LockRecord ( BPTR fh , ULONG offset , ULONG length , ULONG mode , ULONG timeout ) ;
 BOOL LockRecords ( const struct RecordLock * recArray , ULONG timeout ) ;
 BOOL UnLockRecord ( BPTR fh , ULONG offset , ULONG length ) ;
 BOOL UnLockRecords ( const struct RecordLock * recArray ) ;

 BPTR SelectInput ( BPTR fh ) ;
 BPTR SelectOutput ( BPTR fh ) ;
 LONG FGetC ( BPTR fh ) ;
 LONG FPutC ( BPTR fh , LONG ch ) ;
 LONG UnGetC ( BPTR fh , LONG character ) ;
 LONG FRead ( BPTR fh , APTR block , ULONG blocklen , ULONG number ) ;
 LONG FWrite ( BPTR fh , CONST_APTR block , ULONG blocklen , ULONG number ) ;
 STRPTR FGets ( BPTR fh , STRPTR buf , ULONG buflen ) ;
 LONG FPuts ( BPTR fh , CONST_STRPTR str ) ;
 void VFWritef ( BPTR fh , CONST_STRPTR format , const LONG * argarray ) ;
 void FWritef ( BPTR fh , CONST_STRPTR format , ... ) ;
 LONG VFPrintf ( BPTR fh , CONST_STRPTR format , CONST_APTR argarray ) ;
 LONG FPrintf ( BPTR fh , CONST_STRPTR format , ... ) ;
 LONG Flush ( BPTR fh ) ;
 LONG SetVBuf ( BPTR fh , STRPTR buff , LONG type , LONG size ) ;

 BPTR DupLockFromFH ( BPTR fh ) ;
 BPTR OpenFromLock ( BPTR lock ) ;
 BPTR ParentOfFH ( BPTR fh ) ;
 BOOL ExamineFH ( BPTR fh , struct FileInfoBlock * fib ) ;
 LONG SetFileDate ( CONST_STRPTR name , const struct DateStamp * date ) ;
 LONG NameFromLock ( BPTR lock , STRPTR buffer , LONG len ) ;
 LONG NameFromFH ( BPTR fh , STRPTR buffer , LONG len ) ;
 WORD SplitName ( CONST_STRPTR name , ULONG separator , STRPTR buf , LONG oldpos , LONG size ) ;
 LONG SameLock ( BPTR lock1 , BPTR lock2 ) ;
 LONG SetMode ( BPTR fh , LONG mode ) ;
 LONG ExAll ( BPTR lock , struct ExAllData * buffer , LONG size , LONG data , struct ExAllControl * control ) ;
 LONG ReadLink ( struct MsgPort * port , BPTR lock , CONST_STRPTR path , STRPTR buffer , ULONG size ) ;
 LONG MakeLink ( CONST_STRPTR name , LONG dest , LONG soft ) ;
 LONG ChangeMode ( LONG type , BPTR fh , LONG newmode ) ;
 LONG SetFileSize ( BPTR fh , LONG pos , LONG mode ) ;

 LONG SetIoErr ( LONG result ) ;
 BOOL Fault ( LONG code , CONST_STRPTR header , STRPTR buffer , LONG len ) ;
 BOOL PrintFault ( LONG code , CONST_STRPTR header ) ;
 LONG ErrorReport ( LONG code , LONG type , ULONG arg1 , struct MsgPort * device ) ;

 struct CommandLineInterface * Cli ( void ) ;
 struct Process * CreateNewProc ( const struct TagItem * tags ) ;
 struct Process * CreateNewProcTagList ( const struct TagItem * tags ) ;
 struct Process * CreateNewProcTags ( ULONG tag1type , ... ) ;
 LONG RunCommand ( BPTR seg , LONG stack , CONST_STRPTR paramptr , LONG paramlen ) ;
 struct MsgPort * GetConsoleTask ( void ) ;
 struct MsgPort * SetConsoleTask ( struct MsgPort * task ) ;
 struct MsgPort * GetFileSysTask ( void ) ;
 struct MsgPort * SetFileSysTask ( struct MsgPort * task ) ;
 STRPTR GetArgStr ( void ) ;
 STRPTR SetArgStr ( STRPTR string ) ;
 struct Process * FindCliProc ( ULONG num ) ;
 ULONG MaxCli ( void ) ;
 BOOL SetCurrentDirName ( CONST_STRPTR name ) ;
 BOOL GetCurrentDirName ( STRPTR buf , LONG len ) ;
 BOOL SetProgramName ( CONST_STRPTR name ) ;
 BOOL GetProgramName ( STRPTR buf , LONG len ) ;
 BOOL SetPrompt ( CONST_STRPTR name ) ;
 BOOL GetPrompt ( STRPTR buf , LONG len ) ;
 BPTR SetProgramDir ( BPTR lock ) ;
 BPTR GetProgramDir ( void ) ;

 LONG SystemTagList ( CONST_STRPTR command , const struct TagItem * tags ) ;
 LONG System ( CONST_STRPTR command , const struct TagItem * tags ) ;
 LONG SystemTags ( CONST_STRPTR command , ULONG tag1type , ... ) ;
 LONG AssignLock ( CONST_STRPTR name , BPTR lock ) ;
 BOOL AssignLate ( CONST_STRPTR name , CONST_STRPTR path ) ;
 BOOL AssignPath ( CONST_STRPTR name , CONST_STRPTR path ) ;
 BOOL AssignAdd ( CONST_STRPTR name , BPTR lock ) ;
 LONG RemAssignList ( CONST_STRPTR name , BPTR lock ) ;
 struct DevProc * GetDeviceProc ( CONST_STRPTR name , struct DevProc * dp ) ;
 void FreeDeviceProc ( struct DevProc * dp ) ;
 struct DosList * LockDosList ( ULONG flags ) ;
 void UnLockDosList ( ULONG flags ) ;
 struct DosList * AttemptLockDosList ( ULONG flags ) ;
 BOOL RemDosEntry ( struct DosList * dlist ) ;
 LONG AddDosEntry ( struct DosList * dlist ) ;
 struct DosList * FindDosEntry ( const struct DosList * dlist , CONST_STRPTR name , ULONG flags ) ;
 struct DosList * NextDosEntry ( const struct DosList * dlist , ULONG flags ) ;
 struct DosList * MakeDosEntry ( CONST_STRPTR name , LONG type ) ;
 void FreeDosEntry ( struct DosList * dlist ) ;
 BOOL IsFileSystem ( CONST_STRPTR name ) ;

 BOOL Format ( CONST_STRPTR filesystem , CONST_STRPTR volumename , ULONG dostype ) ;
 LONG Relabel ( CONST_STRPTR drive , CONST_STRPTR newname ) ;
 LONG Inhibit ( CONST_STRPTR name , LONG onoff ) ;
 LONG AddBuffers ( CONST_STRPTR name , LONG number ) ;

 LONG CompareDates ( const struct DateStamp * date1 , const struct DateStamp * date2 ) ;
 LONG DateToStr ( struct DateTime * datetime ) ;
 LONG StrToDate ( struct DateTime * datetime ) ;

 BPTR InternalLoadSeg ( BPTR fh , BPTR table , const LONG * funcarray , LONG * stack ) ;
 BOOL InternalUnLoadSeg ( BPTR seglist , void ( * freefunc ) ( ) ) ;
 BPTR NewLoadSeg ( CONST_STRPTR file , const struct TagItem * tags ) ;
 BPTR NewLoadSegTagList ( CONST_STRPTR file , const struct TagItem * tags ) ;
 BPTR NewLoadSegTags ( CONST_STRPTR file , ULONG tag1type , ... ) ;
 LONG AddSegment ( CONST_STRPTR name , BPTR seg , LONG system ) ;
 struct Segment * FindSegment ( CONST_STRPTR name , const struct Segment * seg , LONG system ) ;
 LONG RemSegment ( struct Segment * seg ) ;

 LONG CheckSignal ( LONG mask ) ;
 struct RDArgs * ReadArgs ( CONST_STRPTR arg_template , LONG * array , struct RDArgs * args ) ;
 LONG FindArg ( CONST_STRPTR keyword , CONST_STRPTR arg_template ) ;
 LONG ReadItem ( CONST_STRPTR name , LONG maxchars , struct CSource * cSource ) ;
 LONG StrToLong ( CONST_STRPTR string , LONG * value ) ;
 LONG MatchFirst ( CONST_STRPTR pat , struct AnchorPath * anchor ) ;
 LONG MatchNext ( struct AnchorPath * anchor ) ;
 void MatchEnd ( struct AnchorPath * anchor ) ;
 LONG ParsePattern ( CONST_STRPTR pat , UBYTE * patbuf , LONG patbuflen ) ;
 BOOL MatchPattern ( const UBYTE * patbuf , CONST_STRPTR str ) ;
 void FreeArgs ( struct RDArgs * args ) ;
 STRPTR FilePart ( CONST_STRPTR path ) ;
 STRPTR PathPart ( CONST_STRPTR path ) ;
 BOOL AddPart ( STRPTR dirname , CONST_STRPTR filename , ULONG size ) ;

 BOOL StartNotify ( struct NotifyRequest * notify ) ;
 void EndNotify ( struct NotifyRequest * notify ) ;

 BOOL SetVar ( CONST_STRPTR name , CONST_STRPTR buffer , LONG size , LONG flags ) ;
 LONG GetVar ( CONST_STRPTR name , STRPTR buffer , LONG size , LONG flags ) ;
 LONG DeleteVar ( CONST_STRPTR name , ULONG flags ) ;
 struct LocalVar * FindVar ( CONST_STRPTR name , ULONG type ) ;
 LONG CliInitNewcli ( struct DosPacket * dp ) ;
 LONG CliInitRun ( struct DosPacket * dp ) ;
 LONG WriteChars ( CONST_STRPTR buf , ULONG buflen ) ;
 LONG PutStr ( CONST_STRPTR str ) ;
 LONG VPrintf ( CONST_STRPTR format , CONST_APTR argarray ) ;
 LONG Printf ( CONST_STRPTR format , ... ) ;

 LONG ParsePatternNoCase ( CONST_STRPTR pat , UBYTE * patbuf , LONG patbuflen ) ;
 BOOL MatchPatternNoCase ( const UBYTE * patbuf , CONST_STRPTR str ) ;

 BOOL SameDevice ( BPTR lock1 , BPTR lock2 ) ;





 void ExAllEnd ( BPTR lock , struct ExAllData * buffer , LONG size , LONG data , struct ExAllControl * control ) ;
 BOOL SetOwner ( CONST_STRPTR name , LONG owner_info ) ;

 LONG VolumeRequestHook ( CONST_STRPTR vol ) ;
 BPTR GetCurrentDir ( void ) ;
 LONG PutErrStr ( CONST_STRPTR str ) ;
 LONG ErrorOutput ( void ) ;
 LONG SelectError ( BPTR fh ) ;
 APTR DoShellMethodTagList ( ULONG method , const struct TagItem * tags ) ;
 APTR DoShellMethod ( ULONG method , ULONG tag1type , ... ) ;
 LONG ScanStackToken ( BPTR seg , LONG defaultstack ) ;
#line 10 "proto\dos.h"
#pragma stdargs-off



 extern struct DosLibrary * DOSBase ;
#line 8 "inline\dos_protos.h"
 BPTR __Open ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG accessMode ) = "\tjsr\t-30(a6)" ;


 LONG __Close ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR file ) = "\tjsr\t-36(a6)" ;


 LONG __Read ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR file , __reg ( "d2" ) APTR buffer , __reg ( "d3" ) LONG length ) = "\tjsr\t-42(a6)" ;


 LONG __Write ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR file , __reg ( "d2" ) CONST_APTR buffer , __reg ( "d3" ) LONG length ) = "\tjsr\t-48(a6)" ;


 BPTR __Input ( __reg ( "a6" ) void * ) = "\tjsr\t-54(a6)" ;


 BPTR __Output ( __reg ( "a6" ) void * ) = "\tjsr\t-60(a6)" ;


 LONG __Seek ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR file , __reg ( "d2" ) LONG position , __reg ( "d3" ) LONG offset ) = "\tjsr\t-66(a6)" ;


 LONG __DeleteFile ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-72(a6)" ;


 LONG __Rename ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR oldName , __reg ( "d2" ) CONST_STRPTR newName ) = "\tjsr\t-78(a6)" ;


 BPTR __Lock ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG type ) = "\tjsr\t-84(a6)" ;


 void __UnLock ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock ) = "\tjsr\t-90(a6)" ;


 BPTR __DupLock ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock ) = "\tjsr\t-96(a6)" ;


 LONG __Examine ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock , __reg ( "d2" ) struct FileInfoBlock * fileInfoBlock ) = "\tjsr\t-102(a6)" ;


 LONG __ExNext ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock , __reg ( "d2" ) struct FileInfoBlock * fileInfoBlock ) = "\tjsr\t-108(a6)" ;


 LONG __Info ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock , __reg ( "d2" ) struct InfoData * parameterBlock ) = "\tjsr\t-114(a6)" ;


 BPTR __CreateDir ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-120(a6)" ;


 BPTR __CurrentDir ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock ) = "\tjsr\t-126(a6)" ;


 LONG __IoErr ( __reg ( "a6" ) void * ) = "\tjsr\t-132(a6)" ;


 struct MsgPort * __CreateProc ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG pri , __reg ( "d3" ) BPTR segList , __reg ( "d4" ) LONG stackSize ) = "\tjsr\t-138(a6)" ;


 void __Exit ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG returnCode ) = "\tjsr\t-144(a6)" ;


 BPTR __LoadSeg ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-150(a6)" ;


 void __UnLoadSeg ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR seglist ) = "\tjsr\t-156(a6)" ;


 struct MsgPort * __DeviceProc ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-174(a6)" ;


 LONG __SetComment ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) CONST_STRPTR comment ) = "\tjsr\t-180(a6)" ;


 LONG __SetProtection ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG protect ) = "\tjsr\t-186(a6)" ;


 struct DateStamp * __DateStamp ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DateStamp * date ) = "\tjsr\t-192(a6)" ;


 void __Delay ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG timeout ) = "\tjsr\t-198(a6)" ;


 LONG __WaitForChar ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR file , __reg ( "d2" ) LONG timeout ) = "\tjsr\t-204(a6)" ;


 BPTR __ParentDir ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock ) = "\tjsr\t-210(a6)" ;


 LONG __IsInteractive ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR file ) = "\tjsr\t-216(a6)" ;


 LONG __Execute ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR string , __reg ( "d2" ) BPTR file , __reg ( "d3" ) BPTR file2 ) = "\tjsr\t-222(a6)" ;


 APTR __AllocDosObject ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG type , __reg ( "d2" ) const struct TagItem * tags ) = "\tjsr\t-228(a6)" ;


 APTR __AllocDosObjectTagList ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG type , __reg ( "d2" ) const struct TagItem * tags ) = "\tjsr\t-228(a6)" ;



 APTR __AllocDosObjectTags ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG type , ULONG tags , ... ) = "\tmove.l\td2,-(a7)\n\tmove.l\ta7,d2\n\taddq.l\t#4,d2\n\tjsr\t-228(a6)\n\tmove.l\t(a7)+,d2" ;



 void __FreeDosObject ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG type , __reg ( "d2" ) APTR ptr ) = "\tjsr\t-234(a6)" ;


 LONG __DoPkt ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) LONG action , __reg ( "d3" ) LONG arg1 , __reg ( "d4" ) LONG arg2 , __reg ( "d5" ) LONG arg3 , __reg ( "d6" ) LONG arg4 , __reg ( "d7" ) LONG arg5 ) = "\tjsr\t-240(a6)" ;


 LONG __DoPkt0 ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) LONG action ) = "\tjsr\t-240(a6)" ;


 LONG __DoPkt1 ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) LONG action , __reg ( "d3" ) LONG arg1 ) = "\tjsr\t-240(a6)" ;


 LONG __DoPkt2 ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) LONG action , __reg ( "d3" ) LONG arg1 , __reg ( "d4" ) LONG arg2 ) = "\tjsr\t-240(a6)" ;


 LONG __DoPkt3 ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) LONG action , __reg ( "d3" ) LONG arg1 , __reg ( "d4" ) LONG arg2 , __reg ( "d5" ) LONG arg3 ) = "\tjsr\t-240(a6)" ;


 LONG __DoPkt4 ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) LONG action , __reg ( "d3" ) LONG arg1 , __reg ( "d4" ) LONG arg2 , __reg ( "d5" ) LONG arg3 , __reg ( "d6" ) LONG arg4 ) = "\tjsr\t-240(a6)" ;


 void __SendPkt ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DosPacket * dp , __reg ( "d2" ) struct MsgPort * port , __reg ( "d3" ) struct MsgPort * replyport ) = "\tjsr\t-246(a6)" ;


 struct DosPacket * __WaitPkt ( __reg ( "a6" ) void * ) = "\tjsr\t-252(a6)" ;


 void __ReplyPkt ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DosPacket * dp , __reg ( "d2" ) LONG res1 , __reg ( "d3" ) LONG res2 ) = "\tjsr\t-258(a6)" ;


 void __AbortPkt ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) struct DosPacket * pkt ) = "\tjsr\t-264(a6)" ;


 BOOL __LockRecord ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) ULONG offset , __reg ( "d3" ) ULONG length , __reg ( "d4" ) ULONG mode , __reg ( "d5" ) ULONG timeout ) = "\tjsr\t-270(a6)" ;


 BOOL __LockRecords ( __reg ( "a6" ) void * , __reg ( "d1" ) const struct RecordLock * recArray , __reg ( "d2" ) ULONG timeout ) = "\tjsr\t-276(a6)" ;


 BOOL __UnLockRecord ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) ULONG offset , __reg ( "d3" ) ULONG length ) = "\tjsr\t-282(a6)" ;


 BOOL __UnLockRecords ( __reg ( "a6" ) void * , __reg ( "d1" ) const struct RecordLock * recArray ) = "\tjsr\t-288(a6)" ;


 BPTR __SelectInput ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh ) = "\tjsr\t-294(a6)" ;


 BPTR __SelectOutput ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh ) = "\tjsr\t-300(a6)" ;


 LONG __FGetC ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh ) = "\tjsr\t-306(a6)" ;


 LONG __FPutC ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) LONG ch ) = "\tjsr\t-312(a6)" ;


 LONG __UnGetC ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) LONG character ) = "\tjsr\t-318(a6)" ;


 LONG __FRead ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) APTR block , __reg ( "d3" ) ULONG blocklen , __reg ( "d4" ) ULONG number ) = "\tjsr\t-324(a6)" ;


 LONG __FWrite ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) CONST_APTR block , __reg ( "d3" ) ULONG blocklen , __reg ( "d4" ) ULONG number ) = "\tjsr\t-330(a6)" ;


 STRPTR __FGets ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) STRPTR buf , __reg ( "d3" ) ULONG buflen ) = "\tjsr\t-336(a6)" ;


 LONG __FPuts ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) CONST_STRPTR str ) = "\tjsr\t-342(a6)" ;


 void __VFWritef ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) CONST_STRPTR format , __reg ( "d3" ) const LONG * argarray ) = "\tjsr\t-348(a6)" ;



 void __FWritef ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) CONST_STRPTR format , ... ) = "\tmove.l\td3,-(a7)\n\tmove.l\ta7,d3\n\taddq.l\t#4,d3\n\tjsr\t-348(a6)\n\tmove.l\t(a7)+,d3" ;



 LONG __VFPrintf ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) CONST_STRPTR format , __reg ( "d3" ) CONST_APTR argarray ) = "\tjsr\t-354(a6)" ;



 LONG __FPrintf ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) CONST_STRPTR format , ... ) = "\tmove.l\td3,-(a7)\n\tmove.l\ta7,d3\n\taddq.l\t#4,d3\n\tjsr\t-354(a6)\n\tmove.l\t(a7)+,d3" ;



 LONG __Flush ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh ) = "\tjsr\t-360(a6)" ;


 LONG __SetVBuf ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) STRPTR buff , __reg ( "d3" ) LONG type , __reg ( "d4" ) LONG size ) = "\tjsr\t-366(a6)" ;


 BPTR __DupLockFromFH ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh ) = "\tjsr\t-372(a6)" ;


 BPTR __OpenFromLock ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock ) = "\tjsr\t-378(a6)" ;


 BPTR __ParentOfFH ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh ) = "\tjsr\t-384(a6)" ;


 BOOL __ExamineFH ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) struct FileInfoBlock * fib ) = "\tjsr\t-390(a6)" ;


 LONG __SetFileDate ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) const struct DateStamp * date ) = "\tjsr\t-396(a6)" ;


 LONG __NameFromLock ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock , __reg ( "d2" ) STRPTR buffer , __reg ( "d3" ) LONG len ) = "\tjsr\t-402(a6)" ;


 LONG __NameFromFH ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) STRPTR buffer , __reg ( "d3" ) LONG len ) = "\tjsr\t-408(a6)" ;


 WORD __SplitName ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) ULONG separator , __reg ( "d3" ) STRPTR buf , __reg ( "d4" ) LONG oldpos , __reg ( "d5" ) LONG size ) = "\tjsr\t-414(a6)" ;


 LONG __SameLock ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock1 , __reg ( "d2" ) BPTR lock2 ) = "\tjsr\t-420(a6)" ;


 LONG __SetMode ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) LONG mode ) = "\tjsr\t-426(a6)" ;


 LONG __ExAll ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock , __reg ( "d2" ) struct ExAllData * buffer , __reg ( "d3" ) LONG size , __reg ( "d4" ) LONG data , __reg ( "d5" ) struct ExAllControl * control ) = "\tjsr\t-432(a6)" ;


 LONG __ReadLink ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * port , __reg ( "d2" ) BPTR lock , __reg ( "d3" ) CONST_STRPTR path , __reg ( "d4" ) STRPTR buffer , __reg ( "d5" ) ULONG size ) = "\tjsr\t-438(a6)" ;


 LONG __MakeLink ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG dest , __reg ( "d3" ) LONG soft ) = "\tjsr\t-444(a6)" ;


 LONG __ChangeMode ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG type , __reg ( "d2" ) BPTR fh , __reg ( "d3" ) LONG newmode ) = "\tjsr\t-450(a6)" ;


 LONG __SetFileSize ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh , __reg ( "d2" ) LONG pos , __reg ( "d3" ) LONG mode ) = "\tjsr\t-456(a6)" ;


 LONG __SetIoErr ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG result ) = "\tjsr\t-462(a6)" ;


 BOOL __Fault ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG code , __reg ( "d2" ) CONST_STRPTR header , __reg ( "d3" ) STRPTR buffer , __reg ( "d4" ) LONG len ) = "\tjsr\t-468(a6)" ;


 BOOL __PrintFault ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG code , __reg ( "d2" ) CONST_STRPTR header ) = "\tjsr\t-474(a6)" ;


 LONG __ErrorReport ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG code , __reg ( "d2" ) LONG type , __reg ( "d3" ) ULONG arg1 , __reg ( "d4" ) struct MsgPort * device ) = "\tjsr\t-480(a6)" ;


 struct CommandLineInterface * __Cli ( __reg ( "a6" ) void * ) = "\tjsr\t-492(a6)" ;


 struct Process * __CreateNewProc ( __reg ( "a6" ) void * , __reg ( "d1" ) const struct TagItem * tags ) = "\tjsr\t-498(a6)" ;


 struct Process * __CreateNewProcTagList ( __reg ( "a6" ) void * , __reg ( "d1" ) const struct TagItem * tags ) = "\tjsr\t-498(a6)" ;



 struct Process * __CreateNewProcTags ( __reg ( "a6" ) void * , ULONG tags , ... ) = "\tmove.l\td1,-(a7)\n\tmove.l\ta7,d1\n\taddq.l\t#4,d1\n\tjsr\t-498(a6)\n\tmove.l\t(a7)+,d1" ;



 LONG __RunCommand ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR seg , __reg ( "d2" ) LONG stack , __reg ( "d3" ) CONST_STRPTR paramptr , __reg ( "d4" ) LONG paramlen ) = "\tjsr\t-504(a6)" ;


 struct MsgPort * __GetConsoleTask ( __reg ( "a6" ) void * ) = "\tjsr\t-510(a6)" ;


 struct MsgPort * __SetConsoleTask ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * task ) = "\tjsr\t-516(a6)" ;


 struct MsgPort * __GetFileSysTask ( __reg ( "a6" ) void * ) = "\tjsr\t-522(a6)" ;


 struct MsgPort * __SetFileSysTask ( __reg ( "a6" ) void * , __reg ( "d1" ) struct MsgPort * task ) = "\tjsr\t-528(a6)" ;


 STRPTR __GetArgStr ( __reg ( "a6" ) void * ) = "\tjsr\t-534(a6)" ;


 STRPTR __SetArgStr ( __reg ( "a6" ) void * , __reg ( "d1" ) STRPTR string ) = "\tjsr\t-540(a6)" ;


 struct Process * __FindCliProc ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG num ) = "\tjsr\t-546(a6)" ;


 ULONG __MaxCli ( __reg ( "a6" ) void * ) = "\tjsr\t-552(a6)" ;


 BOOL __SetCurrentDirName ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-558(a6)" ;


 BOOL __GetCurrentDirName ( __reg ( "a6" ) void * , __reg ( "d1" ) STRPTR buf , __reg ( "d2" ) LONG len ) = "\tjsr\t-564(a6)" ;


 BOOL __SetProgramName ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-570(a6)" ;


 BOOL __GetProgramName ( __reg ( "a6" ) void * , __reg ( "d1" ) STRPTR buf , __reg ( "d2" ) LONG len ) = "\tjsr\t-576(a6)" ;


 BOOL __SetPrompt ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-582(a6)" ;


 BOOL __GetPrompt ( __reg ( "a6" ) void * , __reg ( "d1" ) STRPTR buf , __reg ( "d2" ) LONG len ) = "\tjsr\t-588(a6)" ;


 BPTR __SetProgramDir ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock ) = "\tjsr\t-594(a6)" ;


 BPTR __GetProgramDir ( __reg ( "a6" ) void * ) = "\tjsr\t-600(a6)" ;


 LONG __SystemTagList ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR command , __reg ( "d2" ) const struct TagItem * tags ) = "\tjsr\t-606(a6)" ;


 LONG __System ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR command , __reg ( "d2" ) const struct TagItem * tags ) = "\tjsr\t-606(a6)" ;



 LONG __SystemTags ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR command , ULONG tags , ... ) = "\tmove.l\td2,-(a7)\n\tmove.l\ta7,d2\n\taddq.l\t#4,d2\n\tjsr\t-606(a6)\n\tmove.l\t(a7)+,d2" ;



 LONG __AssignLock ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) BPTR lock ) = "\tjsr\t-612(a6)" ;


 BOOL __AssignLate ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) CONST_STRPTR path ) = "\tjsr\t-618(a6)" ;


 BOOL __AssignPath ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) CONST_STRPTR path ) = "\tjsr\t-624(a6)" ;


 BOOL __AssignAdd ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) BPTR lock ) = "\tjsr\t-630(a6)" ;


 LONG __RemAssignList ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) BPTR lock ) = "\tjsr\t-636(a6)" ;


 struct DevProc * __GetDeviceProc ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) struct DevProc * dp ) = "\tjsr\t-642(a6)" ;


 void __FreeDeviceProc ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DevProc * dp ) = "\tjsr\t-648(a6)" ;


 struct DosList * __LockDosList ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG flags ) = "\tjsr\t-654(a6)" ;


 void __UnLockDosList ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG flags ) = "\tjsr\t-660(a6)" ;


 struct DosList * __AttemptLockDosList ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG flags ) = "\tjsr\t-666(a6)" ;


 BOOL __RemDosEntry ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DosList * dlist ) = "\tjsr\t-672(a6)" ;


 LONG __AddDosEntry ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DosList * dlist ) = "\tjsr\t-678(a6)" ;


 struct DosList * __FindDosEntry ( __reg ( "a6" ) void * , __reg ( "d1" ) const struct DosList * dlist , __reg ( "d2" ) CONST_STRPTR name , __reg ( "d3" ) ULONG flags ) = "\tjsr\t-684(a6)" ;


 struct DosList * __NextDosEntry ( __reg ( "a6" ) void * , __reg ( "d1" ) const struct DosList * dlist , __reg ( "d2" ) ULONG flags ) = "\tjsr\t-690(a6)" ;


 struct DosList * __MakeDosEntry ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG type ) = "\tjsr\t-696(a6)" ;


 void __FreeDosEntry ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DosList * dlist ) = "\tjsr\t-702(a6)" ;


 BOOL __IsFileSystem ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name ) = "\tjsr\t-708(a6)" ;


 BOOL __Format ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR filesystem , __reg ( "d2" ) CONST_STRPTR volumename , __reg ( "d3" ) ULONG dostype ) = "\tjsr\t-714(a6)" ;


 LONG __Relabel ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR drive , __reg ( "d2" ) CONST_STRPTR newname ) = "\tjsr\t-720(a6)" ;


 LONG __Inhibit ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG onoff ) = "\tjsr\t-726(a6)" ;


 LONG __AddBuffers ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG number ) = "\tjsr\t-732(a6)" ;


 LONG __CompareDates ( __reg ( "a6" ) void * , __reg ( "d1" ) const struct DateStamp * date1 , __reg ( "d2" ) const struct DateStamp * date2 ) = "\tjsr\t-738(a6)" ;


 LONG __DateToStr ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DateTime * datetime ) = "\tjsr\t-744(a6)" ;


 LONG __StrToDate ( __reg ( "a6" ) void * , __reg ( "d1" ) struct DateTime * datetime ) = "\tjsr\t-750(a6)" ;


 BPTR __InternalLoadSeg ( __reg ( "a6" ) void * , __reg ( "d0" ) BPTR fh , __reg ( "a0" ) void * table , __reg ( "a1" ) const LONG * funcarray , __reg ( "a2" ) LONG * stack ) = "\tjsr\t-756(a6)" ;


 BOOL __InternalUnLoadSeg ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR seglist , __reg ( "a1" ) void ( * freefunc ) ( ) ) = "\tjsr\t-762(a6)" ;


 BPTR __NewLoadSeg ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR file , __reg ( "d2" ) const struct TagItem * tags ) = "\tjsr\t-768(a6)" ;


 BPTR __NewLoadSegTagList ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR file , __reg ( "d2" ) const struct TagItem * tags ) = "\tjsr\t-768(a6)" ;



 BPTR __NewLoadSegTags ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR file , ULONG tags , ... ) = "\tmove.l\td2,-(a7)\n\tmove.l\ta7,d2\n\taddq.l\t#4,d2\n\tjsr\t-768(a6)\n\tmove.l\t(a7)+,d2" ;



 LONG __AddSegment ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) BPTR seg , __reg ( "d3" ) LONG system ) = "\tjsr\t-774(a6)" ;


 struct Segment * __FindSegment ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) const struct Segment * seg , __reg ( "d3" ) LONG system ) = "\tjsr\t-780(a6)" ;


 LONG __RemSegment ( __reg ( "a6" ) void * , __reg ( "d1" ) struct Segment * seg ) = "\tjsr\t-786(a6)" ;


 LONG __CheckSignal ( __reg ( "a6" ) void * , __reg ( "d1" ) LONG mask ) = "\tjsr\t-792(a6)" ;


 struct RDArgs * __ReadArgs ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR arg_template , __reg ( "d2" ) LONG * array , __reg ( "d3" ) struct RDArgs * args ) = "\tjsr\t-798(a6)" ;


 LONG __FindArg ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR keyword , __reg ( "d2" ) CONST_STRPTR arg_template ) = "\tjsr\t-804(a6)" ;


 LONG __ReadItem ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG maxchars , __reg ( "d3" ) struct CSource * cSource ) = "\tjsr\t-810(a6)" ;


 LONG __StrToLong ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR string , __reg ( "d2" ) LONG * value ) = "\tjsr\t-816(a6)" ;


 LONG __MatchFirst ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR pat , __reg ( "d2" ) struct AnchorPath * anchor ) = "\tjsr\t-822(a6)" ;


 LONG __MatchNext ( __reg ( "a6" ) void * , __reg ( "d1" ) struct AnchorPath * anchor ) = "\tjsr\t-828(a6)" ;


 void __MatchEnd ( __reg ( "a6" ) void * , __reg ( "d1" ) struct AnchorPath * anchor ) = "\tjsr\t-834(a6)" ;


 LONG __ParsePattern ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR pat , __reg ( "d2" ) UBYTE * patbuf , __reg ( "d3" ) LONG patbuflen ) = "\tjsr\t-840(a6)" ;


 BOOL __MatchPattern ( __reg ( "a6" ) void * , __reg ( "d1" ) const UBYTE * patbuf , __reg ( "d2" ) CONST_STRPTR str ) = "\tjsr\t-846(a6)" ;


 void __FreeArgs ( __reg ( "a6" ) void * , __reg ( "d1" ) struct RDArgs * args ) = "\tjsr\t-858(a6)" ;


 STRPTR __FilePart ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR path ) = "\tjsr\t-870(a6)" ;


 STRPTR __PathPart ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR path ) = "\tjsr\t-876(a6)" ;


 BOOL __AddPart ( __reg ( "a6" ) void * , __reg ( "d1" ) STRPTR dirname , __reg ( "d2" ) CONST_STRPTR filename , __reg ( "d3" ) ULONG size ) = "\tjsr\t-882(a6)" ;


 BOOL __StartNotify ( __reg ( "a6" ) void * , __reg ( "d1" ) struct NotifyRequest * notify ) = "\tjsr\t-888(a6)" ;


 void __EndNotify ( __reg ( "a6" ) void * , __reg ( "d1" ) struct NotifyRequest * notify ) = "\tjsr\t-894(a6)" ;


 BOOL __SetVar ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) CONST_STRPTR buffer , __reg ( "d3" ) LONG size , __reg ( "d4" ) LONG flags ) = "\tjsr\t-900(a6)" ;


 LONG __GetVar ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) STRPTR buffer , __reg ( "d3" ) LONG size , __reg ( "d4" ) LONG flags ) = "\tjsr\t-906(a6)" ;


 LONG __DeleteVar ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) ULONG flags ) = "\tjsr\t-912(a6)" ;


 struct LocalVar * __FindVar ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) ULONG type ) = "\tjsr\t-918(a6)" ;


 LONG __CliInitNewcli ( __reg ( "a6" ) void * , __reg ( "a0" ) struct DosPacket * dp ) = "\tjsr\t-930(a6)" ;


 LONG __CliInitRun ( __reg ( "a6" ) void * , __reg ( "a0" ) struct DosPacket * dp ) = "\tjsr\t-936(a6)" ;


 LONG __WriteChars ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR buf , __reg ( "d2" ) ULONG buflen ) = "\tjsr\t-942(a6)" ;


 LONG __PutStr ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR str ) = "\tjsr\t-948(a6)" ;


 LONG __VPrintf ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR format , __reg ( "d2" ) CONST_APTR argarray ) = "\tjsr\t-954(a6)" ;



 LONG __Printf ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR format , ... ) = "\tmove.l\td2,-(a7)\n\tmove.l\ta7,d2\n\taddq.l\t#4,d2\n\tjsr\t-954(a6)\n\tmove.l\t(a7)+,d2" ;



 LONG __ParsePatternNoCase ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR pat , __reg ( "d2" ) UBYTE * patbuf , __reg ( "d3" ) LONG patbuflen ) = "\tjsr\t-966(a6)" ;


 BOOL __MatchPatternNoCase ( __reg ( "a6" ) void * , __reg ( "d1" ) const UBYTE * patbuf , __reg ( "d2" ) CONST_STRPTR str ) = "\tjsr\t-972(a6)" ;


 BOOL __SameDevice ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock1 , __reg ( "d2" ) BPTR lock2 ) = "\tjsr\t-984(a6)" ;


 void __ExAllEnd ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR lock , __reg ( "d2" ) struct ExAllData * buffer , __reg ( "d3" ) LONG size , __reg ( "d4" ) LONG data , __reg ( "d5" ) struct ExAllControl * control ) = "\tjsr\t-990(a6)" ;


 BOOL __SetOwner ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR name , __reg ( "d2" ) LONG owner_info ) = "\tjsr\t-996(a6)" ;


 LONG __VolumeRequestHook ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR vol ) = "\tjsr\t-1014(a6)" ;


 BPTR __GetCurrentDir ( __reg ( "a6" ) void * ) = "\tjsr\t-1026(a6)" ;


 LONG __PutErrStr ( __reg ( "a6" ) void * , __reg ( "d1" ) CONST_STRPTR str ) = "\tjsr\t-1128(a6)" ;


 LONG __ErrorOutput ( __reg ( "a6" ) void * ) = "\tjsr\t-1134(a6)" ;


 LONG __SelectError ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR fh ) = "\tjsr\t-1140(a6)" ;


 APTR __DoShellMethodTagList ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG method , __reg ( "a0" ) const struct TagItem * tags ) = "\tjsr\t-1152(a6)" ;



 APTR __DoShellMethod ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG method , ULONG tags , ... ) = "\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-1152(a6)\n\tmovea.l\t(a7)+,a0" ;



 LONG __ScanStackToken ( __reg ( "a6" ) void * , __reg ( "d1" ) BPTR seg , __reg ( "d2" ) LONG defaultstack ) = "\tjsr\t-1158(a6)" ;
#line 6 "time.h"
 typedef long time_t ;




 typedef long clock_t ;










 struct tm {
 int tm_sec ;
 int tm_min ;
 int tm_hour ;
 int tm_mday ;
 int tm_mon ;
 int tm_year ;
 int tm_wday ;
 int tm_yday ;
 int tm_isdst ;


 } ;

 time_t time ( time_t * ) ;
 double difftime ( time_t , time_t ) ;
 char * ctime ( const time_t * ) ;
 char * asctime ( const struct tm * ) ;
 clock_t clock ( void ) ;
 struct tm * gmtime ( const time_t * ) ;
 struct tm * localtime ( const time_t * ) ;
 time_t mktime ( struct tm * ) ;
 size_t strftime ( char * , size_t , const char * , const struct tm * ) ;
#line 27 "..\writeLog.h"
 typedef enum {
 LOG_GENERAL ,
 LOG_MEMORY ,
 LOG_GUI ,
 LOG_ICONS ,
 LOG_BACKUP ,
 LOG_ERRORS ,
 LOG_CATEGORY_COUNT
 } LogCategory ;





 typedef enum {
 LOG_LEVEL_DEBUG ,
 LOG_LEVEL_INFO ,
 LOG_LEVEL_WARNING ,
 LOG_LEVEL_ERROR
 } LogLevel ;






 void initialize_log_system ( BOOL cleanOldLogs ) ;


 void shutdown_log_system ( void ) ;


 void log_message ( LogCategory category , LogLevel level , const char * format , ... ) ;












 void enable_log_category ( LogCategory category , BOOL enable ) ;


 BOOL is_log_category_enabled ( LogCategory category ) ;


 void set_log_level ( LogCategory category , LogLevel minLevel ) ;






 void append_to_log ( const char * format , ... ) ;


 void initialize_logfile ( void ) ;
 void delete_logfile ( void ) ;





 void reset_log_performance_stats ( void ) ;
 void print_log_performance_stats ( void ) ;






 void set_global_log_level ( LogLevel minLevel ) ;


 LogLevel get_global_log_level ( void ) ;


 void set_memory_logging_enabled ( BOOL enabled ) ;


 BOOL is_memory_logging_enabled ( void ) ;


 void set_performance_logging_enabled ( BOOL enabled ) ;


 BOOL is_performance_logging_enabled ( void ) ;






 const char * get_log_category_name ( LogCategory category ) ;


 const char * get_log_level_name ( LogLevel level ) ;
#line 42 "graphics\gfx.h"
 struct Rectangle
 {
 WORD MinX , MinY ;
 WORD MaxX , MaxY ;
 } ;

 struct Rect32
 {
 LONG MinX , MinY ;
 LONG MaxX , MaxY ;
 } ;

 typedef struct tPoint
 {
 WORD x , y ;
 } Point ;


 typedef UBYTE * PLANEPTR ;

 struct BitMap
 {
 UWORD BytesPerRow ;
 UWORD Rows ;
 UBYTE Flags ;
 UBYTE Depth ;
 UWORD pad ;
 PLANEPTR Planes [ 8 ] ;
 } ;
#line 20 "graphics\rastport.h"
 struct AreaInfo
 {
 WORD * VctrTbl ;
 WORD * VctrPtr ;
 BYTE * FlagTbl ;
 BYTE * FlagPtr ;
 WORD Count ;
 WORD MaxCount ;
 WORD FirstX , FirstY ;
 } ;

 struct TmpRas
 {
 BYTE * RasPtr ;
 LONG Size ;
 } ;


 struct GelsInfo
 {
 BYTE sprRsrvd ;

 UBYTE Flags ;
 struct VSprite * gelHead , * gelTail ;

 WORD * nextLine ;

 WORD * * lastColor ;
 struct collTable * collHandler ;
 WORD leftmost , rightmost , topmost , bottommost ;
 APTR firstBlissObj , lastBlissObj ;
 } ;

 struct RastPort
 {
 struct Layer * Layer ;
 struct BitMap * BitMap ;
 UWORD * AreaPtrn ;
 struct TmpRas * TmpRas ;
 struct AreaInfo * AreaInfo ;
 struct GelsInfo * GelsInfo ;
 UBYTE Mask ;
 BYTE FgPen ;
 BYTE BgPen ;
 BYTE AOlPen ;
 BYTE DrawMode ;
 BYTE AreaPtSz ;
 BYTE linpatcnt ;
 BYTE dummy ;
 UWORD Flags ;
 UWORD LinePtrn ;
 WORD cp_x , cp_y ;
 UBYTE minterms [ 8 ] ;
 WORD PenWidth ;
 WORD PenHeight ;
 struct TextFont * Font ;
 UBYTE AlgoStyle ;
 UBYTE TxFlags ;
 UWORD TxHeight ;
 UWORD TxWidth ;
 UWORD TxBaseline ;
 WORD TxSpacing ;
 APTR * RP_User ;
 ULONG longreserved [ 2 ] ;

 UWORD wordreserved [ 7 ] ;
 UBYTE reserved [ 8 ] ;

 } ;
#line 52 "..\path_utilities.h"
 void iTidy_TruncatePathMiddle ( const char * path , char * output , int max_chars ) ;



























 void iTidy_TruncatePathMiddlePixels ( struct RastPort * rp ,
 const char * path ,
 char * output ,
 UWORD max_width ) ;











































 BOOL iTidy_ShortenPathWithParentDir ( const char * path ,
 char * output ,
 int max_chars ) ;
#line 19 "exec\memory.h"
 struct MemChunk {
 struct MemChunk * mc_Next ;
 ULONG mc_Bytes ;
 } ;




 struct MemHeader {
 struct Node mh_Node ;
 UWORD mh_Attributes ;
 struct MemChunk * mh_First ;
 APTR mh_Lower ;
 APTR mh_Upper ;
 ULONG mh_Free ;
 } ;




 struct MemEntry {
 union {
 ULONG meu_Reqs ;
 APTR meu_Addr ;
 } me_Un ;
 ULONG me_Length ;
 } ;









 struct MemList {
 struct Node ml_Node ;
 UWORD ml_NumEntries ;
 struct MemEntry ml_ME [ 1 ] ;
 } ;





























 struct MemHandlerData
 {
 ULONG memh_RequestSize ;
 ULONG memh_RequestFlags ;
 ULONG memh_Flags ;
 } ;
#line 8 "proto\exec.h"
#pragma stdargs-on
#line 21 "exec\interrupts.h"
 struct Interrupt {
 struct Node is_Node ;
 APTR is_Data ;
 void ( * is_Code ) ( ) ;
 } ;


 struct IntVector {
 APTR iv_Data ;
 void ( * iv_Code ) ( ) ;
 struct Node * iv_Node ;
 } ;


 struct SoftIntList {
 struct List sh_List ;
 UWORD sh_Pad ;
 } ;
#line 33 "exec\execbase.h"
 struct ExecBase {
 struct Library LibNode ;



 UWORD SoftVer ;
 WORD LowMemChkSum ;
 ULONG ChkBase ;
 APTR ColdCapture ;
 APTR CoolCapture ;
 APTR WarmCapture ;
 APTR SysStkUpper ;
 APTR SysStkLower ;
 ULONG MaxLocMem ;
 APTR DebugEntry ;
 APTR DebugData ;
 APTR AlertData ;
 APTR MaxExtMem ;

 UWORD ChkSum ;



 struct IntVector IntVects [ 16 ] ;



 struct Task * ThisTask ;

 ULONG IdleCount ;
 ULONG DispCount ;
 UWORD Quantum ;
 UWORD Elapsed ;
 UWORD SysFlags ;
 BYTE IDNestCnt ;
 BYTE TDNestCnt ;

 UWORD AttnFlags ;

 UWORD AttnResched ;
 APTR ResModules ;
 APTR TaskTrapCode ;
 APTR TaskExceptCode ;
 APTR TaskExitCode ;
 ULONG TaskSigAlloc ;
 UWORD TaskTrapAlloc ;




 struct List MemList ;
 struct List ResourceList ;
 struct List DeviceList ;
 struct List IntrList ;
 struct List LibList ;
 struct List PortList ;
 struct List TaskReady ;
 struct List TaskWait ;

 struct SoftIntList SoftInts [ 5 ] ;



 LONG LastAlert [ 4 ] ;











 UBYTE VBlankFrequency ;
 UBYTE PowerSupplyFrequency ;

 struct List SemaphoreList ;







 APTR KickMemPtr ;
 APTR KickTagPtr ;
 APTR KickCheckSum ;



 UWORD ex_Pad0 ;
 ULONG ex_LaunchPoint ;
 APTR ex_RamLibPrivate ;





 ULONG ex_EClockFrequency ;
 ULONG ex_CacheControl ;
 ULONG ex_TaskID ;

 ULONG ex_Reserved1 [ 5 ] ;

 APTR ex_MMULock ;

 ULONG ex_Reserved2 [ 3 ] ;






 struct MinList ex_MemHandlers ;
 APTR ex_MemHandler ;
 } ;
#line 24 "exec\devices.h"
 struct Device {
 struct Library dd_Library ;
 } ;




 struct Unit {
 struct MsgPort unit_MsgPort ;

 UBYTE unit_flags ;
 UBYTE unit_pad ;
 UWORD unit_OpenCnt ;
 } ;
#line 17 "exec\resident.h"
 struct Resident {
 UWORD rt_MatchWord ;
 struct Resident * rt_MatchTag ;
 APTR rt_EndSkip ;
 UBYTE rt_Flags ;
 UBYTE rt_Version ;
 UBYTE rt_Type ;
 BYTE rt_Pri ;
 char * rt_Name ;
 char * rt_IdString ;
 APTR rt_Init ;
 } ;
#line 45 "clib\exec_protos.h"
 ULONG Supervisor ( ULONG ( * userFunction ) ( ) ) ;


 void InitCode ( ULONG startClass , ULONG version ) ;
 void InitStruct ( CONST_APTR initTable , APTR memory , ULONG size ) ;
 struct Library * MakeLibrary ( CONST_APTR funcInit , CONST_APTR structInit , ULONG ( * libInit ) ( ) , ULONG dataSize , ULONG segList ) ;
 void MakeFunctions ( APTR target , CONST_APTR functionArray , ULONG funcDispBase ) ;
 struct Resident * FindResident ( CONST_STRPTR name ) ;
 APTR InitResident ( const struct Resident * resident , ULONG segList ) ;

 void Alert ( ULONG alertNum ) ;
 void Debug ( ULONG flags ) ;

 void Disable ( void ) ;
 void Enable ( void ) ;
 void Forbid ( void ) ;
 void Permit ( void ) ;
 ULONG SetSR ( ULONG newSR , ULONG mask ) ;
 APTR SuperState ( void ) ;
 void UserState ( APTR sysStack ) ;
 struct Interrupt * SetIntVector ( LONG intNumber , struct Interrupt * interrupt ) ;
 void AddIntServer ( LONG intNumber , struct Interrupt * interrupt ) ;
 void RemIntServer ( LONG intNumber , struct Interrupt * interrupt ) ;
 void Cause ( const struct Interrupt * interrupt ) ;

 APTR Allocate ( struct MemHeader * freeList , ULONG byteSize ) ;
 void Deallocate ( struct MemHeader * freeList , APTR memoryBlock , ULONG byteSize ) ;
 APTR AllocMem ( ULONG byteSize , ULONG requirements ) ;
 APTR AllocAbs ( ULONG byteSize , APTR location ) ;
 void FreeMem ( APTR memoryBlock , ULONG byteSize ) ;
 ULONG AvailMem ( ULONG requirements ) ;
 struct MemList * AllocEntry ( const struct MemList * entry ) ;
 void FreeEntry ( struct MemList * entry ) ;

 void Insert ( struct List * list , struct Node * node , struct Node * pred ) ;
 void InsertMinNode ( struct MinList * minlist , struct MinNode * minnode , struct MinNode * minpred ) ;
 void AddHead ( struct List * list , struct Node * node ) ;
 void AddHeadMinList ( struct MinList * minlist , struct MinNode * minnode ) ;
 void AddTail ( struct List * list , struct Node * node ) ;
 void AddTailMinList ( struct MinList * minlist , struct MinNode * minnode ) ;
 void Remove ( struct Node * node ) ;
 void RemoveMinNode ( struct MinNode * minnode ) ;
 struct Node * RemHead ( struct List * list ) ;
 struct MinNode * RemHeadMinList ( struct MinList * minlist ) ;
 struct Node * RemTail ( struct List * list ) ;
 struct MinNode * RemTailMinList ( struct MinList * minlist ) ;
 void Enqueue ( struct List * list , struct Node * node ) ;
 struct Node * FindName ( struct List * list , CONST_STRPTR name ) ;

 APTR AddTask ( struct Task * task , APTR initPC , APTR finalPC ) ;
 void RemTask ( struct Task * task ) ;
 struct Task * FindTask ( CONST_STRPTR name ) ;
 BYTE SetTaskPri ( struct Task * task , LONG priority ) ;
 ULONG SetSignal ( ULONG newSignals , ULONG signalSet ) ;
 ULONG SetExcept ( ULONG newSignals , ULONG signalSet ) ;
 ULONG Wait ( ULONG signalSet ) ;
 void Signal ( struct Task * task , ULONG signalSet ) ;
 BYTE AllocSignal ( LONG signalNum ) ;
 void FreeSignal ( LONG signalNum ) ;
 LONG AllocTrap ( LONG trapNum ) ;
 void FreeTrap ( LONG trapNum ) ;

 void AddPort ( struct MsgPort * port ) ;
 void RemPort ( struct MsgPort * port ) ;
 void PutMsg ( struct MsgPort * port , struct Message * message ) ;
 struct Message * GetMsg ( struct MsgPort * port ) ;
 void ReplyMsg ( struct Message * message ) ;
 struct Message * WaitPort ( struct MsgPort * port ) ;
 struct MsgPort * FindPort ( CONST_STRPTR name ) ;

 void AddLibrary ( struct Library * library ) ;
 void RemLibrary ( struct Library * library ) ;
 struct Library * OldOpenLibrary ( CONST_STRPTR libName ) ;
 void CloseLibrary ( struct Library * library ) ;
 APTR SetFunction ( struct Library * library , LONG funcOffset , ULONG ( * newFunction ) ( ) ) ;
 void SumLibrary ( struct Library * library ) ;

 void AddDevice ( struct Device * device ) ;
 void RemDevice ( struct Device * device ) ;
 BYTE OpenDevice ( CONST_STRPTR devName , ULONG unit , struct IORequest * ioRequest , ULONG flags ) ;
 void CloseDevice ( struct IORequest * ioRequest ) ;
 BYTE DoIO ( struct IORequest * ioRequest ) ;
 void SendIO ( struct IORequest * ioRequest ) ;
 struct IORequest * CheckIO ( const struct IORequest * ioRequest ) ;
 BYTE WaitIO ( struct IORequest * ioRequest ) ;
 void AbortIO ( struct IORequest * ioRequest ) ;

 void AddResource ( APTR resource ) ;
 void RemResource ( APTR resource ) ;
 APTR OpenResource ( CONST_STRPTR resName ) ;


 APTR RawDoFmt ( CONST_STRPTR formatString , APTR dataStream , void ( * putChProc ) ( ) , APTR putChData ) ;
 ULONG GetCC ( void ) ;
 ULONG TypeOfMem ( CONST_APTR address ) ;
 ULONG Procure ( struct SignalSemaphore * sigSem , struct SemaphoreMessage * bidMsg ) ;
 void Vacate ( struct SignalSemaphore * sigSem , struct SemaphoreMessage * bidMsg ) ;
 struct Library * OpenLibrary ( CONST_STRPTR libName , ULONG version ) ;


 void InitSemaphore ( struct SignalSemaphore * sigSem ) ;
 void ObtainSemaphore ( struct SignalSemaphore * sigSem ) ;
 void ReleaseSemaphore ( struct SignalSemaphore * sigSem ) ;
 ULONG AttemptSemaphore ( struct SignalSemaphore * sigSem ) ;
 void ObtainSemaphoreList ( struct List * sigSem ) ;
 void ReleaseSemaphoreList ( struct List * sigSem ) ;
 struct SignalSemaphore * FindSemaphore ( CONST_STRPTR name ) ;
 void AddSemaphore ( struct SignalSemaphore * sigSem ) ;
 void RemSemaphore ( struct SignalSemaphore * sigSem ) ;

 ULONG SumKickData ( void ) ;

 void AddMemList ( ULONG size , ULONG attributes , LONG pri , APTR base , STRPTR name ) ;
 void CopyMem ( CONST_APTR source , APTR dest , ULONG size ) ;
 void CopyMemQuick ( CONST_APTR source , APTR dest , ULONG size ) ;


 void CacheClearU ( void ) ;
 void CacheClearE ( APTR address , ULONG length , ULONG caches ) ;
 ULONG CacheControl ( ULONG cacheBits , ULONG cacheMask ) ;

 APTR CreateIORequest ( struct MsgPort * port , ULONG size ) ;
 void DeleteIORequest ( APTR iorequest ) ;
 struct MsgPort * CreateMsgPort ( void ) ;
 void DeleteMsgPort ( struct MsgPort * port ) ;
 void ObtainSemaphoreShared ( struct SignalSemaphore * sigSem ) ;

 APTR AllocVec ( ULONG byteSize , ULONG requirements ) ;
 void FreeVec ( APTR memoryBlock ) ;

 APTR CreatePool ( ULONG requirements , ULONG puddleSize , ULONG threshSize ) ;
 void DeletePool ( APTR poolHeader ) ;
 APTR AllocPooled ( APTR poolHeader , ULONG memSize ) ;
 void FreePooled ( APTR poolHeader , APTR memory , ULONG memSize ) ;

 ULONG AttemptSemaphoreShared ( struct SignalSemaphore * sigSem ) ;
 void ColdReboot ( void ) ;
 void StackSwap ( struct StackSwapStruct * newStack ) ;


 APTR CachePreDMA ( CONST_APTR address , ULONG * length , ULONG flags ) ;
 void CachePostDMA ( CONST_APTR address , ULONG * length , ULONG flags ) ;



 void AddMemHandler ( struct Interrupt * memhand ) ;
 void RemMemHandler ( struct Interrupt * memhand ) ;

 ULONG ObtainQuickVector ( APTR interruptCode ) ;


 void NewMinList ( struct MinList * minlist ) ;
#line 10 "proto\exec.h"
#pragma stdargs-off



 extern struct ExecBase * SysBase ;
#line 8 "inline\exec_protos.h"
 ULONG __Supervisor ( __reg ( "a6" ) void * , __reg ( "a5" ) ULONG ( * userFunction ) ( ) ) = "\tjsr\t-30(a6)" ;


 void __InitCode ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG startClass , __reg ( "d1" ) ULONG version ) = "\tjsr\t-72(a6)" ;


 void __InitStruct ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_APTR initTable , __reg ( "a2" ) APTR memory , __reg ( "d0" ) ULONG size ) = "\tjsr\t-78(a6)" ;


 struct Library * __MakeLibrary ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_APTR funcInit , __reg ( "a1" ) CONST_APTR structInit , __reg ( "a2" ) ULONG ( * libInit ) ( ) , __reg ( "d0" ) ULONG dataSize , __reg ( "d1" ) ULONG segList ) = "\tjsr\t-84(a6)" ;


 void __MakeFunctions ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR target , __reg ( "a1" ) CONST_APTR functionArray , __reg ( "a2" ) void * funcDispBase ) = "\tjsr\t-90(a6)" ;


 struct Resident * __FindResident ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_STRPTR name ) = "\tjsr\t-96(a6)" ;


 APTR __InitResident ( __reg ( "a6" ) void * , __reg ( "a1" ) const struct Resident * resident , __reg ( "d1" ) ULONG segList ) = "\tjsr\t-102(a6)" ;


 void __Alert ( __reg ( "a6" ) void * , __reg ( "d7" ) ULONG alertNum ) = "\tjsr\t-108(a6)" ;


 void __Debug ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG flags ) = "\tjsr\t-114(a6)" ;


 void __Disable ( __reg ( "a6" ) void * ) = "\tjsr\t-120(a6)" ;


 void __Enable ( __reg ( "a6" ) void * ) = "\tjsr\t-126(a6)" ;


 void __Forbid ( __reg ( "a6" ) void * ) = "\tjsr\t-132(a6)" ;


 void __Permit ( __reg ( "a6" ) void * ) = "\tjsr\t-138(a6)" ;


 ULONG __SetSR ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG newSR , __reg ( "d1" ) ULONG mask ) = "\tjsr\t-144(a6)" ;


 APTR __SuperState ( __reg ( "a6" ) void * ) = "\tjsr\t-150(a6)" ;


 void __UserState ( __reg ( "a6" ) void * , __reg ( "d0" ) APTR sysStack ) = "\tjsr\t-156(a6)" ;


 struct Interrupt * __SetIntVector ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG intNumber , __reg ( "a1" ) struct Interrupt * interrupt ) = "\tjsr\t-162(a6)" ;


 void __AddIntServer ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG intNumber , __reg ( "a1" ) struct Interrupt * interrupt ) = "\tjsr\t-168(a6)" ;


 void __RemIntServer ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG intNumber , __reg ( "a1" ) struct Interrupt * interrupt ) = "\tjsr\t-174(a6)" ;


 void __Cause ( __reg ( "a6" ) void * , __reg ( "a1" ) const struct Interrupt * interrupt ) = "\tjsr\t-180(a6)" ;


 APTR __Allocate ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MemHeader * freeList , __reg ( "d0" ) ULONG byteSize ) = "\tjsr\t-186(a6)" ;


 void __Deallocate ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MemHeader * freeList , __reg ( "a1" ) APTR memoryBlock , __reg ( "d0" ) ULONG byteSize ) = "\tjsr\t-192(a6)" ;


 APTR __AllocMem ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG byteSize , __reg ( "d1" ) ULONG requirements ) = "\tjsr\t-198(a6)" ;


 APTR __AllocAbs ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG byteSize , __reg ( "a1" ) APTR location ) = "\tjsr\t-204(a6)" ;


 void __FreeMem ( __reg ( "a6" ) void * , __reg ( "a1" ) APTR memoryBlock , __reg ( "d0" ) ULONG byteSize ) = "\tjsr\t-210(a6)" ;


 ULONG __AvailMem ( __reg ( "a6" ) void * , __reg ( "d1" ) ULONG requirements ) = "\tjsr\t-216(a6)" ;


 struct MemList * __AllocEntry ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct MemList * entry ) = "\tjsr\t-222(a6)" ;


 void __FreeEntry ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MemList * entry ) = "\tjsr\t-228(a6)" ;


 void __Insert ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * list , __reg ( "a1" ) struct Node * node , __reg ( "a2" ) struct Node * pred ) = "\tjsr\t-234(a6)" ;


 void __InsertMinNode ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MinList * minlist , __reg ( "a1" ) struct MinNode * minnode , __reg ( "a2" ) struct MinNode * minpred ) = "\tjsr\t-234(a6)" ;


 void __AddHead ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * list , __reg ( "a1" ) struct Node * node ) = "\tjsr\t-240(a6)" ;


 void __AddHeadMinList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MinList * minlist , __reg ( "a1" ) struct MinNode * minnode ) = "\tjsr\t-240(a6)" ;


 void __AddTail ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * list , __reg ( "a1" ) struct Node * node ) = "\tjsr\t-246(a6)" ;


 void __AddTailMinList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MinList * minlist , __reg ( "a1" ) struct MinNode * minnode ) = "\tjsr\t-246(a6)" ;


 void __Remove ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Node * node ) = "\tjsr\t-252(a6)" ;


 void __RemoveMinNode ( __reg ( "a6" ) void * , __reg ( "a1" ) struct MinNode * minnode ) = "\tjsr\t-252(a6)" ;


 struct Node * __RemHead ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * list ) = "\tjsr\t-258(a6)" ;


 struct MinNode * __RemHeadMinList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MinList * minlist ) = "\tjsr\t-258(a6)" ;


 struct Node * __RemTail ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * list ) = "\tjsr\t-264(a6)" ;


 struct MinNode * __RemTailMinList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MinList * minlist ) = "\tjsr\t-264(a6)" ;


 void __Enqueue ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * list , __reg ( "a1" ) struct Node * node ) = "\tjsr\t-270(a6)" ;


 struct Node * __FindName ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * list , __reg ( "a1" ) CONST_STRPTR name ) = "\tjsr\t-276(a6)" ;


 APTR __AddTask ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Task * task , __reg ( "a2" ) APTR initPC , __reg ( "a3" ) APTR finalPC ) = "\tjsr\t-282(a6)" ;


 void __RemTask ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Task * task ) = "\tjsr\t-288(a6)" ;


 struct Task * __FindTask ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_STRPTR name ) = "\tjsr\t-294(a6)" ;


 BYTE __SetTaskPri ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Task * task , __reg ( "d0" ) LONG priority ) = "\tjsr\t-300(a6)" ;


 ULONG __SetSignal ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG newSignals , __reg ( "d1" ) ULONG signalSet ) = "\tjsr\t-306(a6)" ;


 ULONG __SetExcept ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG newSignals , __reg ( "d1" ) ULONG signalSet ) = "\tjsr\t-312(a6)" ;


 ULONG __Wait ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG signalSet ) = "\tjsr\t-318(a6)" ;


 void __Signal ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Task * task , __reg ( "d0" ) ULONG signalSet ) = "\tjsr\t-324(a6)" ;


 BYTE __AllocSignal ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG signalNum ) = "\tjsr\t-330(a6)" ;


 void __FreeSignal ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG signalNum ) = "\tjsr\t-336(a6)" ;


 LONG __AllocTrap ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG trapNum ) = "\tjsr\t-342(a6)" ;


 void __FreeTrap ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG trapNum ) = "\tjsr\t-348(a6)" ;


 void __AddPort ( __reg ( "a6" ) void * , __reg ( "a1" ) struct MsgPort * port ) = "\tjsr\t-354(a6)" ;


 void __RemPort ( __reg ( "a6" ) void * , __reg ( "a1" ) struct MsgPort * port ) = "\tjsr\t-360(a6)" ;


 void __PutMsg ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MsgPort * port , __reg ( "a1" ) struct Message * message ) = "\tjsr\t-366(a6)" ;


 struct Message * __GetMsg ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MsgPort * port ) = "\tjsr\t-372(a6)" ;


 void __ReplyMsg ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Message * message ) = "\tjsr\t-378(a6)" ;


 struct Message * __WaitPort ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MsgPort * port ) = "\tjsr\t-384(a6)" ;


 struct MsgPort * __FindPort ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_STRPTR name ) = "\tjsr\t-390(a6)" ;


 void __AddLibrary ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Library * library ) = "\tjsr\t-396(a6)" ;


 void __RemLibrary ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Library * library ) = "\tjsr\t-402(a6)" ;


 struct Library * __OldOpenLibrary ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_STRPTR libName ) = "\tjsr\t-408(a6)" ;


 void __CloseLibrary ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Library * library ) = "\tjsr\t-414(a6)" ;


 APTR __SetFunction ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Library * library , __reg ( "a0" ) void * funcOffset , __reg ( "d0" ) ULONG ( * newFunction ) ( ) ) = "\tjsr\t-420(a6)" ;


 void __SumLibrary ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Library * library ) = "\tjsr\t-426(a6)" ;


 void __AddDevice ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Device * device ) = "\tjsr\t-432(a6)" ;


 void __RemDevice ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Device * device ) = "\tjsr\t-438(a6)" ;


 BYTE __OpenDevice ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_STRPTR devName , __reg ( "d0" ) ULONG unit , __reg ( "a1" ) struct IORequest * ioRequest , __reg ( "d1" ) ULONG flags ) = "\tjsr\t-444(a6)" ;


 void __CloseDevice ( __reg ( "a6" ) void * , __reg ( "a1" ) struct IORequest * ioRequest ) = "\tjsr\t-450(a6)" ;


 BYTE __DoIO ( __reg ( "a6" ) void * , __reg ( "a1" ) struct IORequest * ioRequest ) = "\tjsr\t-456(a6)" ;


 void __SendIO ( __reg ( "a6" ) void * , __reg ( "a1" ) struct IORequest * ioRequest ) = "\tjsr\t-462(a6)" ;


 struct IORequest * __CheckIO ( __reg ( "a6" ) void * , __reg ( "a1" ) const struct IORequest * ioRequest ) = "\tjsr\t-468(a6)" ;


 BYTE __WaitIO ( __reg ( "a6" ) void * , __reg ( "a1" ) struct IORequest * ioRequest ) = "\tjsr\t-474(a6)" ;


 void __AbortIO ( __reg ( "a6" ) void * , __reg ( "a1" ) struct IORequest * ioRequest ) = "\tjsr\t-480(a6)" ;


 void __AddResource ( __reg ( "a6" ) void * , __reg ( "a1" ) APTR resource ) = "\tjsr\t-486(a6)" ;


 void __RemResource ( __reg ( "a6" ) void * , __reg ( "a1" ) APTR resource ) = "\tjsr\t-492(a6)" ;


 APTR __OpenResource ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_STRPTR resName ) = "\tjsr\t-498(a6)" ;


 APTR __RawDoFmt ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_STRPTR formatString , __reg ( "a1" ) APTR dataStream , __reg ( "a2" ) void ( * putChProc ) ( ) , __reg ( "a3" ) APTR putChData ) = "\tjsr\t-522(a6)" ;


 ULONG __GetCC ( __reg ( "a6" ) void * ) = "\tjsr\t-528(a6)" ;


 ULONG __TypeOfMem ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_APTR address ) = "\tjsr\t-534(a6)" ;


 ULONG __Procure ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem , __reg ( "a1" ) struct SemaphoreMessage * bidMsg ) = "\tjsr\t-540(a6)" ;


 void __Vacate ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem , __reg ( "a1" ) struct SemaphoreMessage * bidMsg ) = "\tjsr\t-546(a6)" ;


 struct Library * __OpenLibrary ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_STRPTR libName , __reg ( "d0" ) ULONG version ) = "\tjsr\t-552(a6)" ;


 void __InitSemaphore ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-558(a6)" ;


 void __ObtainSemaphore ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-564(a6)" ;


 void __ReleaseSemaphore ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-570(a6)" ;


 ULONG __AttemptSemaphore ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-576(a6)" ;


 void __ObtainSemaphoreList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * sigSem ) = "\tjsr\t-582(a6)" ;


 void __ReleaseSemaphoreList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct List * sigSem ) = "\tjsr\t-588(a6)" ;


 struct SignalSemaphore * __FindSemaphore ( __reg ( "a6" ) void * , __reg ( "a1" ) CONST_STRPTR name ) = "\tjsr\t-594(a6)" ;


 void __AddSemaphore ( __reg ( "a6" ) void * , __reg ( "a1" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-600(a6)" ;


 void __RemSemaphore ( __reg ( "a6" ) void * , __reg ( "a1" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-606(a6)" ;


 ULONG __SumKickData ( __reg ( "a6" ) void * ) = "\tjsr\t-612(a6)" ;


 void __AddMemList ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG size , __reg ( "d1" ) ULONG attributes , __reg ( "d2" ) LONG pri , __reg ( "a0" ) APTR base , __reg ( "a1" ) STRPTR name ) = "\tjsr\t-618(a6)" ;


 void __CopyMem ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_APTR source , __reg ( "a1" ) APTR dest , __reg ( "d0" ) ULONG size ) = "\tjsr\t-624(a6)" ;


 void __CopyMemQuick ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_APTR source , __reg ( "a1" ) APTR dest , __reg ( "d0" ) ULONG size ) = "\tjsr\t-630(a6)" ;


 void __CacheClearU ( __reg ( "a6" ) void * ) = "\tjsr\t-636(a6)" ;


 void __CacheClearE ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR address , __reg ( "d0" ) ULONG length , __reg ( "d1" ) ULONG caches ) = "\tjsr\t-642(a6)" ;


 ULONG __CacheControl ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG cacheBits , __reg ( "d1" ) ULONG cacheMask ) = "\tjsr\t-648(a6)" ;


 APTR __CreateIORequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MsgPort * port , __reg ( "d0" ) ULONG size ) = "\tjsr\t-654(a6)" ;


 void __DeleteIORequest ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR iorequest ) = "\tjsr\t-660(a6)" ;


 struct MsgPort * __CreateMsgPort ( __reg ( "a6" ) void * ) = "\tjsr\t-666(a6)" ;


 void __DeleteMsgPort ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MsgPort * port ) = "\tjsr\t-672(a6)" ;


 void __ObtainSemaphoreShared ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-678(a6)" ;


 APTR __AllocVec ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG byteSize , __reg ( "d1" ) ULONG requirements ) = "\tjsr\t-684(a6)" ;


 void __FreeVec ( __reg ( "a6" ) void * , __reg ( "a1" ) APTR memoryBlock ) = "\tjsr\t-690(a6)" ;


 APTR __CreatePool ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG requirements , __reg ( "d1" ) ULONG puddleSize , __reg ( "d2" ) ULONG threshSize ) = "\tjsr\t-696(a6)" ;


 void __DeletePool ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR poolHeader ) = "\tjsr\t-702(a6)" ;


 APTR __AllocPooled ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR poolHeader , __reg ( "d0" ) ULONG memSize ) = "\tjsr\t-708(a6)" ;


 void __FreePooled ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR poolHeader , __reg ( "a1" ) APTR memory , __reg ( "d0" ) ULONG memSize ) = "\tjsr\t-714(a6)" ;


 ULONG __AttemptSemaphoreShared ( __reg ( "a6" ) void * , __reg ( "a0" ) struct SignalSemaphore * sigSem ) = "\tjsr\t-720(a6)" ;


 void __ColdReboot ( __reg ( "a6" ) void * ) = "\tjsr\t-726(a6)" ;


 void __StackSwap ( __reg ( "a6" ) void * , __reg ( "a0" ) struct StackSwapStruct * newStack ) = "\tjsr\t-732(a6)" ;


 APTR __CachePreDMA ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_APTR address , __reg ( "a1" ) ULONG * length , __reg ( "d0" ) ULONG flags ) = "\tjsr\t-762(a6)" ;


 void __CachePostDMA ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_APTR address , __reg ( "a1" ) ULONG * length , __reg ( "d0" ) ULONG flags ) = "\tjsr\t-768(a6)" ;


 void __AddMemHandler ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Interrupt * memhand ) = "\tjsr\t-774(a6)" ;


 void __RemMemHandler ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Interrupt * memhand ) = "\tjsr\t-780(a6)" ;


 ULONG __ObtainQuickVector ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR interruptCode ) = "\tjsr\t-786(a6)" ;


 void __NewMinList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MinList * minlist ) = "\tjsr\t-828(a6)" ;
#line 8 "proto\intuition.h"
#pragma stdargs-on
#line 41 "graphics\clip.h"
 struct Layer
 {
 struct Layer * front , * back ;
 struct ClipRect * ClipRect ;
 struct RastPort * rp ;
 struct Rectangle bounds ;
 struct Layer * nlink ;



 UWORD priority ;



 UWORD Flags ;
 struct BitMap * SuperBitMap ;
 struct ClipRect * SuperClipRect ;

 APTR Window ;
 WORD Scroll_X , Scroll_Y ;
 struct ClipRect * OnScreen ;
 struct ClipRect * OffScreen ;
 struct ClipRect * Backup ;
 struct ClipRect * SuperSaveClipRects ;
 struct ClipRect * Undamaged ;


 struct Layer_Info * LayerInfo ;
 struct SignalSemaphore Lock ;
 struct Hook * BackFill ;
 ULONG reserved1 ;
 struct Region * ClipRegion ;
 struct ClipRect * clipped ;


 WORD Width , Height ;
 UBYTE reserved2 [ 18 ] ;
 struct Region * DamageList ;
 } ;
















 struct ClipRect
 {
 struct ClipRect * Next ;
 struct ClipRect * reservedlink ;
 LONG obscured ;







 struct BitMap * BitMap ;
 struct Rectangle bounds ;
 struct ClipRect * vlink ;
 struct Layer_Info * home ;








 APTR reserved ;



 } ;
#line 23 "graphics\copper.h"
 struct CopIns
 {
 WORD OpCode ;
 union
 {
 struct CopList * nxtlist ;
 struct
 {
 union
 {
 WORD VWaitPos ;
 WORD DestAddr ;
 } u1 ;
 union
 {
 WORD HWaitPos ;
 WORD DestData ;
 } u2 ;
 } u4 ;
 } u3 ;
 } ;










 struct cprlist
 {
 struct cprlist * Next ;
 UWORD * start ;
 WORD MaxCount ;
 } ;

 struct CopList
 {
 struct CopList * Next ;
 struct CopList * _CopList ;
 struct ViewPort * _ViewPort ;
 struct CopIns * CopIns ;
 struct CopIns * CopPtr ;
 UWORD * CopLStart ;
 UWORD * CopSStart ;
 WORD Count ;
 WORD MaxCount ;
 WORD DyOffset ;






 UWORD SLRepeat ;
 UWORD Flags ;
 } ;






 struct UCopList
 {
 struct UCopList * Next ;
 struct CopList * FirstCopList ;
 struct CopList * CopList ;
 } ;





 struct copinit
 {
 UWORD vsync_hblank [ 2 ] ;
 UWORD diagstrt [ 12 ] ;
 UWORD fm0 [ 2 ] ;
 UWORD diwstart [ 10 ] ;
 UWORD bplcon2 [ 2 ] ;
 UWORD sprfix [ 2 * 8 ] ;
 UWORD sprstrtup [ ( 2 * 8 * 2 ) ] ;
 UWORD wait14 [ 2 ] ;
 UWORD norm_hblank [ 2 ] ;
 UWORD jump [ 2 ] ;
 UWORD wait_forever [ 6 ] ;
 UWORD sprstop [ 8 ] ;
 } ;
#line 21 "graphics\gfxnodes.h"
 struct ExtendedNode {
 struct Node * xln_Succ ;
 struct Node * xln_Pred ;
 UBYTE xln_Type ;
 BYTE xln_Pri ;
 char * xln_Name ;
 UBYTE xln_Subsystem ;
 UBYTE xln_Subtype ;
 struct GfxBase * xln_Library ;




 LONG ( * xln_Init ) ( __reg ( "a0" ) struct ExtendedNode * , __reg ( "d0" ) UWORD ) ;
 } ;
#line 35 "graphics\view.h"
 struct View ;
#line 37 "graphics\monitor.h"
 struct MonitorSpec
 {
 struct ExtendedNode ms_Node ;
 UWORD ms_Flags ;
 LONG ratioh ;
 LONG ratiov ;
 UWORD total_rows ;
 UWORD total_colorclocks ;
 UWORD DeniseMaxDisplayColumn ;
 UWORD BeamCon0 ;
 UWORD min_row ;
 struct SpecialMonitor * ms_Special ;
 WORD ms_OpenCount ;
 LONG ( * ms_transform ) ( __reg ( "a0" ) struct MonitorSpec * , __reg ( "a1" ) Point * , __reg ( "d0" ) UWORD , __reg ( "a2" ) Point * )


 ;
 LONG ( * ms_translate ) ( __reg ( "a0" ) struct MonitorSpec * , __reg ( "a1" ) Point * , __reg ( "d0" ) UWORD , __reg ( "a2" ) Point * )


 ;
 LONG ( * ms_scale ) ( __reg ( "a0" ) struct MonitorSpec * , __reg ( "a1" ) Point * , __reg ( "d0" ) UWORD , __reg ( "a2" ) Point * )


 ;
 UWORD ms_xoffset ;
 UWORD ms_yoffset ;
 struct Rectangle ms_LegalView ;
 LONG ( * ms_maxoscan ) ( __reg ( "a0" ) struct MonitorSpec * , __reg ( "a1" ) struct Rectangle * , __reg ( "d0" ) ULONG )

 ;
 LONG ( * ms_videoscan ) ( __reg ( "a0" ) struct MonitorSpec * , __reg ( "a1" ) struct Rectangle * , __reg ( "d0" ) ULONG )

 ;
 UWORD DeniseMinDisplayColumn ;
 ULONG DisplayCompatible ;
 struct List DisplayInfoDataBase ;
 struct SignalSemaphore DisplayInfoDataBaseSemaphore ;
 LONG ( * ms_MrgCop ) ( struct View * ) ;
 LONG ( * ms_LoadView ) ( struct View * ) ;
 LONG ( * ms_KillView ) ( __reg ( "a0" ) struct MonitorSpec * ) ;
 } ;


































































































 struct AnalogSignalInterval
 {
 UWORD asi_Start ;
 UWORD asi_Stop ;
 } ;

 struct SpecialMonitor
 {
 struct ExtendedNode spm_Node ;
 UWORD spm_Flags ;
 LONG ( * do_monitor ) ( struct MonitorSpec * mspc ) ;
 LONG ( * reserved1 ) ( ) ;
 LONG ( * reserved2 ) ( ) ;
 LONG ( * reserved3 ) ( ) ;
 struct AnalogSignalInterval hblank ;
 struct AnalogSignalInterval vblank ;
 struct AnalogSignalInterval hsync ;
 struct AnalogSignalInterval vsync ;
 } ;
#line 35 "graphics\displayinfo.h"
 typedef APTR DisplayInfoHandle ;









 struct QueryHeader
 {
 ULONG StructID ;
 ULONG DisplayID ;
 ULONG SkipID ;
 ULONG Length ;
 } ;

 struct DisplayInfo
 {
 struct QueryHeader Header ;
 UWORD NotAvailable ;
 ULONG PropertyFlags ;
 Point Resolution ;
 UWORD PixelSpeed ;
 UWORD NumStdSprites ;
 UWORD PaletteRange ;
 Point SpriteResolution ;
 UBYTE pad [ 4 ] ;
 UBYTE RedBits ;
 UBYTE GreenBits ;
 UBYTE BlueBits ;
 UBYTE pad2 [ 5 ] ;
 ULONG reserved [ 2 ] ;
 } ;



















































 struct DimensionInfo
 {
 struct QueryHeader Header ;
 UWORD MaxDepth ;
 UWORD MinRasterWidth ;
 UWORD MinRasterHeight ;
 UWORD MaxRasterWidth ;
 UWORD MaxRasterHeight ;
 struct Rectangle Nominal ;
 struct Rectangle MaxOScan ;
 struct Rectangle VideoOScan ;
 struct Rectangle TxtOScan ;
 struct Rectangle StdOScan ;
 UBYTE pad [ 14 ] ;
 ULONG reserved [ 2 ] ;
 } ;

 struct MonitorInfo
 {
 struct QueryHeader Header ;
 struct MonitorSpec * Mspc ;
 Point ViewPosition ;
 Point ViewResolution ;
 struct Rectangle ViewPositionRange ;
 UWORD TotalRows ;
 UWORD TotalColorClocks ;
 UWORD MinRow ;
 WORD Compatibility ;
 UBYTE pad [ 32 ] ;
 Point MouseTicks ;
 Point DefaultViewPosition ;
 ULONG PreferredModeID ;
 ULONG reserved [ 2 ] ;
 } ;









 struct NameInfo
 {
 struct QueryHeader Header ;
 UBYTE Name [ 32 ] ;
 ULONG reserved [ 2 ] ;
 } ;







 struct VecInfo
 {
 struct QueryHeader Header ;
 APTR Vec ;
 APTR Data ;
 UWORD Type ;
 UWORD pad [ 3 ] ;
 ULONG reserved [ 2 ] ;
 } ;
#line 24 "hardware\custom.h"
 struct Custom {
 UWORD bltddat ;
 UWORD dmaconr ;
 UWORD vposr ;
 UWORD vhposr ;
 UWORD dskdatr ;
 UWORD joy0dat ;
 UWORD joy1dat ;
 UWORD clxdat ;
 UWORD adkconr ;
 UWORD pot0dat ;
 UWORD pot1dat ;
 UWORD potinp ;
 UWORD serdatr ;
 UWORD dskbytr ;
 UWORD intenar ;
 UWORD intreqr ;
 APTR dskpt ;
 UWORD dsklen ;
 UWORD dskdat ;
 UWORD refptr ;
 UWORD vposw ;
 UWORD vhposw ;
 UWORD copcon ;
 UWORD serdat ;
 UWORD serper ;
 UWORD potgo ;
 UWORD joytest ;
 UWORD strequ ;
 UWORD strvbl ;
 UWORD strhor ;
 UWORD strlong ;
 UWORD bltcon0 ;
 UWORD bltcon1 ;
 UWORD bltafwm ;
 UWORD bltalwm ;
 APTR bltcpt ;
 APTR bltbpt ;
 APTR bltapt ;
 APTR bltdpt ;
 UWORD bltsize ;
 UBYTE pad2d ;
 UBYTE bltcon0l ;
 UWORD bltsizv ;
 UWORD bltsizh ;
 UWORD bltcmod ;
 UWORD bltbmod ;
 UWORD bltamod ;
 UWORD bltdmod ;
 UWORD pad34 [ 4 ] ;
 UWORD bltcdat ;
 UWORD bltbdat ;
 UWORD bltadat ;
 UWORD pad3b [ 3 ] ;
 UWORD deniseid ;
 UWORD dsksync ;
 ULONG cop1lc ;
 ULONG cop2lc ;
 UWORD copjmp1 ;
 UWORD copjmp2 ;
 UWORD copins ;
 UWORD diwstrt ;
 UWORD diwstop ;
 UWORD ddfstrt ;
 UWORD ddfstop ;
 UWORD dmacon ;
 UWORD clxcon ;
 UWORD intena ;
 UWORD intreq ;
 UWORD adkcon ;
 struct AudChannel {
 UWORD * ac_ptr ;
 UWORD ac_len ;
 UWORD ac_per ;
 UWORD ac_vol ;
 UWORD ac_dat ;
 UWORD ac_pad [ 2 ] ;
 } aud [ 4 ] ;
 APTR bplpt [ 8 ] ;
 UWORD bplcon0 ;
 UWORD bplcon1 ;
 UWORD bplcon2 ;
 UWORD bplcon3 ;
 UWORD bpl1mod ;
 UWORD bpl2mod ;
 UWORD bplcon4 ;
 UWORD clxcon2 ;
 UWORD bpldat [ 8 ] ;
 APTR sprpt [ 8 ] ;
 struct SpriteDef {
 UWORD pos ;
 UWORD ctl ;
 UWORD dataa ;
 UWORD datab ;
 } spr [ 8 ] ;
 UWORD color [ 32 ] ;
 UWORD htotal ;
 UWORD hsstop ;
 UWORD hbstrt ;
 UWORD hbstop ;
 UWORD vtotal ;
 UWORD vsstop ;
 UWORD vbstrt ;
 UWORD vbstop ;
 UWORD sprhstrt ;
 UWORD sprhstop ;
 UWORD bplhstrt ;
 UWORD bplhstop ;
 UWORD hhposw ;
 UWORD hhposr ;
 UWORD beamcon0 ;
 UWORD hsstrt ;
 UWORD vsstrt ;
 UWORD hcenter ;
 UWORD diwhigh ;
 UWORD padf3 [ 11 ] ;
 UWORD fmode ;
 } ;
#line 49 "graphics\view.h"
 struct ViewPort
 {
 struct ViewPort * Next ;
 struct ColorMap * ColorMap ;

 struct CopList * DspIns ;
 struct CopList * SprIns ;
 struct CopList * ClrIns ;
 struct UCopList * UCopIns ;
 WORD DWidth , DHeight ;
 WORD DxOffset , DyOffset ;
 UWORD Modes ;
 UBYTE SpritePriorities ;
 UBYTE ExtendedModes ;
 struct RasInfo * RasInfo ;
 } ;

 struct View
 {
 struct ViewPort * ViewPort ;
 struct cprlist * LOFCprList ;
 struct cprlist * SHFCprList ;
 WORD DyOffset , DxOffset ;

 UWORD Modes ;
 } ;



 struct ViewExtra
 {
 struct ExtendedNode n ;
 struct View * View ;
 struct MonitorSpec * Monitor ;
 UWORD TopLine ;
 } ;



 struct ViewPortExtra
 {
 struct ExtendedNode n ;
 struct ViewPort * ViewPort ;
 struct Rectangle DisplayClip ;

 APTR VecTable ;
 APTR DriverData [ 2 ] ;
 UWORD Flags ;
 Point Origin [ 2 ] ;


 ULONG cop1ptr ;
 ULONG cop2ptr ;
 } ;



































 struct RasInfo
 {
 struct RasInfo * Next ;
 struct BitMap * BitMap ;
 WORD RxOffset , RyOffset ;
 } ;

 struct ColorMap
 {
 UBYTE Flags ;
 UBYTE Type ;
 UWORD Count ;
 APTR ColorTable ;
 struct ViewPortExtra * cm_vpe ;
 APTR LowColorBits ;
 UBYTE TransparencyPlane ;
 UBYTE SpriteResolution ;
 UBYTE SpriteResDefault ;
 UBYTE AuxFlags ;
 struct ViewPort * cm_vp ;
 APTR NormalDisplayInfo ;
 APTR CoerceDisplayInfo ;
 struct TagItem * cm_batch_items ;
 ULONG VPModeID ;
 struct PaletteExtra * PalExtra ;
 UWORD SpriteBase_Even ;
 UWORD SpriteBase_Odd ;
 UWORD Bp_0_base ;
 UWORD Bp_1_base ;

 } ;











































 struct PaletteExtra
 {
 struct SignalSemaphore pe_Semaphore ;
 UWORD pe_FirstFree ;
 UWORD pe_NFree ;
 UWORD pe_FirstShared ;
 UWORD pe_NShared ;
 UBYTE * pe_RefCnt ;
 UBYTE * pe_AllocList ;
 struct ViewPort * pe_ViewPort ;
 UWORD pe_SharableColors ;
 } ;

























































 struct DBufInfo {
 APTR dbi_Link1 ;
 ULONG dbi_Count1 ;
 struct Message dbi_SafeMessage ;
 APTR dbi_UserData1 ;

 APTR dbi_Link2 ;
 ULONG dbi_Count2 ;
 struct Message dbi_DispMessage ;

 APTR dbi_UserData2 ;
 ULONG dbi_MatchLong ;
 APTR dbi_CopPtr1 ;
 APTR dbi_CopPtr2 ;
 APTR dbi_CopPtr3 ;
 UWORD dbi_BeamPos1 ;
 UWORD dbi_BeamPos2 ;
 } ;
#line 55 "graphics\layers.h"
 struct Layer_Info
 {
 struct Layer * top_layer ;
 void * resPtr1 ;
 void * resPtr2 ;
 struct ClipRect * FreeClipRects ;




 struct Rectangle bounds ;



 struct SignalSemaphore Lock ;
 struct MinList gs_Head ;


 WORD PrivateReserve3 ;
 void * PrivateReserve4 ;
 UWORD Flags ;
 BYTE res_count ;
 BYTE LockLayersCount ;


 BYTE PrivateReserve5 ;
 BYTE UserClipRectsCount ;
 struct Hook * BlankHook ;
 void * resPtr5 ;
 } ;
#line 64 "graphics\text.h"
 struct TextAttr {
 STRPTR ta_Name ;
 UWORD ta_YSize ;
 UBYTE ta_Style ;
 UBYTE ta_Flags ;
 } ;

 struct TTextAttr {
 STRPTR tta_Name ;
 UWORD tta_YSize ;
 UBYTE tta_Style ;
 UBYTE tta_Flags ;
 struct TagItem * tta_Tags ;
 } ;










 struct TextFont {
 struct Message tf_Message ;

 UWORD tf_YSize ;
 UBYTE tf_Style ;
 UBYTE tf_Flags ;
 UWORD tf_XSize ;
 UWORD tf_Baseline ;
 UWORD tf_BoldSmear ;

 UWORD tf_Accessors ;

 UBYTE tf_LoChar ;
 UBYTE tf_HiChar ;
 APTR tf_CharData ;

 UWORD tf_Modulo ;
 APTR tf_CharLoc ;

 APTR tf_CharSpace ;
 APTR tf_CharKern ;
 } ;








 struct TextFontExtension {
 UWORD tfe_MatchWord ;
 UBYTE tfe_Flags0 ;
 UBYTE tfe_Flags1 ;
 struct TextFont * tfe_BackPtr ;
 struct MsgPort * tfe_OrigReplyPort ;
 struct TagItem * tfe_Tags ;
 UWORD * tfe_OFontPatchS ;
 UWORD * tfe_OFontPatchK ;

 } ;













 struct ColorFontColors {
 UWORD cfc_Reserved ;
 UWORD cfc_Count ;
 UWORD * cfc_ColorTable ;
 } ;


 struct ColorTextFont {
 struct TextFont ctf_TF ;
 UWORD ctf_Flags ;
 UBYTE ctf_Depth ;
 UBYTE ctf_FgColor ;
 UBYTE ctf_Low ;
 UBYTE ctf_High ;
 UBYTE ctf_PlanePick ;
 UBYTE ctf_PlaneOnOff ;
 struct ColorFontColors * ctf_ColorFontColors ;
 APTR ctf_CharData [ 8 ] ;
 } ;


 struct TextExtent {
 UWORD te_Width ;
 UWORD te_Height ;
 struct Rectangle te_Extent ;
 } ;
#line 96 "devices\inputevent.h"
 struct IEPointerPixel {
 struct Screen * iepp_Screen ;
 struct {
 WORD X ;
 WORD Y ;
 } iepp_Position ;
 } ;













 struct IEPointerTablet {
 struct {
 UWORD X ;
 UWORD Y ;
 } iept_Range ;
 struct {
 UWORD X ;
 UWORD Y ;
 } iept_Value ;

 WORD iept_Pressure ;
 } ;









 struct IENewTablet
 {








 struct Hook * ient_CallBack ;














 UWORD ient_ScaledX , ient_ScaledY ;
 UWORD ient_ScaledXFraction , ient_ScaledYFraction ;


 ULONG ient_TabletX , ient_TabletY ;




 ULONG ient_RangeX , ient_RangeY ;




 struct TagItem * ient_TagList ;
 } ;














































































 struct InputEvent {
 struct InputEvent * ie_NextEvent ;
 UBYTE ie_Class ;
 UBYTE ie_SubClass ;
 UWORD ie_Code ;
 UWORD ie_Qualifier ;
 union {
 struct {
 WORD ie_x ;
 WORD ie_y ;
 } ie_xy ;
 APTR ie_addr ;
 struct {
 UBYTE ie_prev1DownCode ;
 UBYTE ie_prev1DownQual ;
 UBYTE ie_prev2DownCode ;
 UBYTE ie_prev2DownQual ;
 } ie_dead ;
 } ie_position ;
 TimeVal_Type ie_TimeStamp ;
 } ;
#line 55 "intuition\intuition.h"
 struct Menu
 {
 struct Menu * NextMenu ;
 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;
 UWORD Flags ;
 CONST_STRPTR MenuName ;
 struct MenuItem * FirstItem ;


 WORD JazzX , JazzY , BeatX , BeatY ;
 } ;
















 struct MenuItem
 {
 struct MenuItem * NextItem ;
 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;
 UWORD Flags ;

 LONG MutualExclude ;

 APTR ItemFill ;




 APTR SelectFill ;

 BYTE Command ;

 struct MenuItem * SubItem ;




 UWORD NextSelect ;
 } ;



































 struct Requester
 {
 struct Requester * OlderRequest ;
 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;
 WORD RelLeft , RelTop ;

 struct Gadget * ReqGadget ;
 struct Border * ReqBorder ;
 struct IntuiText * ReqText ;
 UWORD Flags ;


 UBYTE BackFill ;

 struct Layer * ReqLayer ;

 UBYTE ReqPad1 [ 32 ] ;







 struct BitMap * ImageBMap ;
 struct Window * RWindow ;

 struct Image * ReqImage ;

 UBYTE ReqPad2 [ 32 ] ;
 } ;






































 struct Gadget
 {
 struct Gadget * NextGadget ;

 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;

 UWORD Flags ;

 UWORD Activation ;

 UWORD GadgetType ;





 APTR GadgetRender ;




 APTR SelectRender ;

 struct IntuiText * GadgetText ;












 LONG MutualExclude ;




 APTR SpecialInfo ;

 UWORD GadgetID ;
 APTR UserData ;
 } ;


 struct ExtGadget
 {

 struct ExtGadget * NextGadget ;
 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;
 UWORD Flags ;
 UWORD Activation ;
 UWORD GadgetType ;
 APTR GadgetRender ;
 APTR SelectRender ;
 struct IntuiText * GadgetText ;
 LONG MutualExclude ;
 APTR SpecialInfo ;
 UWORD GadgetID ;
 APTR UserData ;


 ULONG MoreFlags ;
 WORD BoundsLeftEdge ;
 WORD BoundsTopEdge ;
 WORD BoundsWidth ;
 WORD BoundsHeight ;
 } ;







































































































































































































































 struct BoolInfo
 {
 UWORD Flags ;
 UWORD * Mask ;





 ULONG Reserved ;
 } ;













 struct PropInfo
 {
 UWORD Flags ;









 UWORD HorizPot ;
 UWORD VertPot ;


















 UWORD HorizBody ;
 UWORD VertBody ;


 UWORD CWidth ;
 UWORD CHeight ;
 UWORD HPotRes , VPotRes ;
 UWORD LeftBorder ;
 UWORD TopBorder ;
 } ;

































 struct StringInfo
 {

 STRPTR Buffer ;
 STRPTR UndoBuffer ;
 WORD BufferPos ;
 WORD MaxChars ;
 WORD DispPos ;


 WORD UndoPos ;
 WORD NumChars ;
 WORD DispCount ;
 WORD CLeft , CTop ;








 struct StringExtend * Extension ;






 LONG LongInt ;






 struct KeyMap * AltKeyMap ;
 } ;








 struct IntuiText
 {
 UBYTE FrontPen , BackPen ;
 UBYTE DrawMode ;
 WORD LeftEdge ;
 WORD TopEdge ;
 const struct TextAttr * ITextFont ;
 STRPTR IText ;
 struct IntuiText * NextText ;
 } ;


















 struct Border
 {
 WORD LeftEdge , TopEdge ;
 UBYTE FrontPen , BackPen ;
 UBYTE DrawMode ;
 BYTE Count ;
 WORD * XY ;
 struct Border * NextBorder ;
 } ;












 struct Image
 {
 WORD LeftEdge ;
 WORD TopEdge ;
 WORD Width ;
 WORD Height ;
 WORD Depth ;
 UWORD * ImageData ;

































 UBYTE PlanePick , PlaneOnOff ;





 struct Image * NextImage ;
 } ;









 struct IntuiMessage
 {
 struct Message ExecMessage ;




 ULONG Class ;


 UWORD Code ;


 UWORD Qualifier ;




 APTR IAddress ;







 WORD MouseX , MouseY ;




 ULONG Seconds , Micros ;




 struct Window * IDCMPWindow ;


 struct IntuiMessage * SpecialLink ;
 } ;


















 struct ExtIntuiMessage
 {
 struct IntuiMessage eim_IntuiMessage ;
 struct TabletData * eim_TabletData ;
 } ;








 struct IntuiWheelData
 {
 UWORD Version ;
 UWORD Reserved ;
 WORD WheelX ;
 WORD WheelY ;
 struct Gadget * HoveredGadget ;
 } ;



 enum
 {
 IMSGCODE_INTUIWHEELDATA = ( 1 << 15 ) ,
 IMSGCODE_INTUIRAWKEYDATA = ( 1 << 14 ) ,
 IMSGCODE_INTUIWHEELDATAREJECT = ( 1 << 13 )
 } ;








































































 struct IBox
 {
 WORD Left ;
 WORD Top ;
 WORD Width ;
 WORD Height ;
 } ;






 struct Window
 {
 struct Window * NextWindow ;

 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;

 WORD MouseY , MouseX ;

 WORD MinWidth , MinHeight ;
 UWORD MaxWidth , MaxHeight ;

 ULONG Flags ;

 struct Menu * MenuStrip ;

 STRPTR Title ;

 struct Requester * FirstRequest ;

 struct Requester * DMRequest ;

 WORD ReqCount ;

 struct Screen * WScreen ;
 struct RastPort * RPort ;











 BYTE BorderLeft , BorderTop , BorderRight , BorderBottom ;
 struct RastPort * BorderRPort ;







 struct Gadget * FirstGadget ;


 struct Window * Parent , * Descendant ;




 UWORD * Pointer ;
 BYTE PtrHeight ;
 BYTE PtrWidth ;
 BYTE XOffset , YOffset ;


 ULONG IDCMPFlags ;
 struct MsgPort * UserPort , * WindowPort ;
 struct IntuiMessage * MessageKey ;

 UBYTE DetailPen , BlockPen ;





 struct Image * CheckMark ;

 STRPTR ScreenTitle ;







 WORD GZZMouseX ;
 WORD GZZMouseY ;



 WORD GZZWidth ;
 WORD GZZHeight ;

 UBYTE * ExtData ;

 BYTE * UserData ;




 struct Layer * WLayer ;




 struct TextFont * IFont ;





 ULONG MoreFlags ;


 } ;




































































 struct NewWindow
 {
 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;

 UBYTE DetailPen , BlockPen ;

 ULONG IDCMPFlags ;

 ULONG Flags ;






 struct Gadget * FirstGadget ;





 struct Image * CheckMark ;

 STRPTR Title ;






 struct Screen * Screen ;





 struct BitMap * BitMap ;













 WORD MinWidth , MinHeight ;
 UWORD MaxWidth , MaxHeight ;






 UWORD Type ;

 } ;








 struct ExtNewWindow
 {
 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;

 UBYTE DetailPen , BlockPen ;
 ULONG IDCMPFlags ;
 ULONG Flags ;
 struct Gadget * FirstGadget ;

 struct Image * CheckMark ;

 STRPTR Title ;
 struct Screen * Screen ;
 struct BitMap * BitMap ;

 WORD MinWidth , MinHeight ;
 UWORD MaxWidth , MaxHeight ;









 UWORD Type ;










 struct TagItem * Extension ;
 } ;
#line 67 "intuition\screens.h"
 struct DrawInfo
 {
 UWORD dri_Version ;
 UWORD dri_NumPens ;
 UWORD * dri_Pens ;

 struct TextFont * dri_Font ;
 UWORD dri_Depth ;

 struct {
 UWORD X ;
 UWORD Y ;
 } dri_Resolution ;

 ULONG dri_Flags ;

 struct Image * dri_CheckMark ;


 struct Image * dri_AmigaKey ;



 struct Screen * dri_Screen ;


 ULONG dri_Reserved [ 4 ] ;
 } ;













































 struct Screen
 {
 struct Screen * NextScreen ;
 struct Window * FirstWindow ;

 WORD LeftEdge , TopEdge ;
 WORD Width , Height ;

 WORD MouseY , MouseX ;

 UWORD Flags ;

 STRPTR Title ;
 STRPTR DefaultTitle ;









 BYTE BarHeight , BarVBorder , BarHBorder , MenuVBorder , MenuHBorder ;
 BYTE WBorTop , WBorLeft , WBorRight , WBorBottom ;

 struct TextAttr * Font ;


 struct ViewPort ViewPort ;
 struct RastPort RastPort ;
 struct BitMap BitMap ;
 struct Layer_Info LayerInfo ;




 struct Gadget * FirstGadget ;

 UBYTE DetailPen , BlockPen ;




 UWORD SaveColor0 ;


 struct Layer * BarLayer ;

 UBYTE * ExtData ;

 UBYTE * UserData ;


 } ;









































































































































































































































































































 struct NewScreen
 {
 WORD LeftEdge , TopEdge , Width , Height , Depth ;

 UBYTE DetailPen , BlockPen ;

 UWORD ViewModes ;

 UWORD Type ;

 struct TextAttr * Font ;

 STRPTR DefaultTitle ;

 struct Gadget * Gadgets ;







 struct BitMap * CustomBitMap ;
 } ;












 struct ExtNewScreen
 {
 WORD LeftEdge , TopEdge , Width , Height , Depth ;
 UBYTE DetailPen , BlockPen ;
 UWORD ViewModes ;
 UWORD Type ;
 struct TextAttr * Font ;
 STRPTR DefaultTitle ;
 struct Gadget * Gadgets ;
 struct BitMap * CustomBitMap ;

 struct TagItem * Extension ;



 } ;




















 struct PubScreenNode {
 struct Node psn_Node ;
 struct Screen * psn_Screen ;
 UWORD psn_Flags ;
 WORD psn_Size ;
 WORD psn_VisitorCount ;
 struct Task * psn_SigTask ;
 UBYTE psn_SigBit ;
 } ;























































































 struct ScreenBuffer
 {
 struct BitMap * sb_BitMap ;
 struct DBufInfo * sb_DBufInfo ;
 } ;
#line 58 "intuition\preferences.h"
 struct Preferences
 {

 BYTE FontHeight ;


 UBYTE PrinterPort ;


 UWORD BaudRate ;


 TimeVal_Type KeyRptSpeed ;
 TimeVal_Type KeyRptDelay ;
 TimeVal_Type DoubleClick ;


 UWORD PointerMatrix [ ( ( 1 + 16 + 1 ) * 2 ) ] ;
 BYTE XOffset ;
 BYTE YOffset ;
 UWORD color17 ;
 UWORD color18 ;
 UWORD color19 ;
 UWORD PointerTicks ;


 UWORD color0 ;
 UWORD color1 ;
 UWORD color2 ;
 UWORD color3 ;


 BYTE ViewXOffset ;
 BYTE ViewYOffset ;
 WORD ViewInitX , ViewInitY ;

 BOOL EnableCLI ;


 UWORD PrinterType ;
 TEXT PrinterFilename [ 30 ] ;


 UWORD PrintPitch ;
 UWORD PrintQuality ;
 UWORD PrintSpacing ;
 UWORD PrintLeftMargin ;
 UWORD PrintRightMargin ;
 UWORD PrintImage ;
 UWORD PrintAspect ;
 UWORD PrintShade ;
 WORD PrintThreshold ;


 UWORD PaperSize ;
 UWORD PaperLength ;
 UWORD PaperType ;



 UBYTE SerRWBits ;

 UBYTE SerStopBuf ;

 UBYTE SerParShk ;

 UBYTE LaceWB ;

 UBYTE Pad [ 12 ] ;
 TEXT PrtDevName [ 16 ] ;


 UBYTE DefaultPrtUnit ;
 UBYTE DefaultSerUnit ;

 BYTE RowSizeChange ;
 BYTE ColumnSizeChange ;

 UWORD PrintFlags ;
 UWORD PrintMaxWidth ;
 UWORD PrintMaxHeight ;
 UBYTE PrintDensity ;
 UBYTE PrintXOffset ;

 UWORD wb_Width ;
 UWORD wb_Height ;
 UBYTE wb_Depth ;

 UBYTE ext_size ;

 } ;
#line 1504 "intuition\intuition.h"
 struct Remember
 {
 struct Remember * NextRemember ;
 ULONG RememberSize ;
 UBYTE * Memory ;
 } ;










 struct ColorSpec
 {
 WORD ColorIndex ;
 UWORD Red ;
 UWORD Green ;
 UWORD Blue ;
 } ;




 struct EasyStruct {
 ULONG es_StructSize ;
 ULONG es_Flags ;
 CONST_STRPTR es_Title ;
 CONST_STRPTR es_TextFormat ;
 CONST_STRPTR es_GadgetFormat ;
 } ;




























































































































































































 struct TabletData
 {



 UWORD td_XFraction , td_YFraction ;


 ULONG td_TabletX , td_TabletY ;




 ULONG td_RangeX , td_RangeY ;




 struct TagItem * td_TagList ;
 } ;

















 struct TabletHookData
 {





 struct Screen * thd_Screen ;




 ULONG thd_Width ;
 ULONG thd_Height ;




 LONG thd_ScreenChanged ;
 } ;
#line 65 "intuition\intuitionbase.h"
 struct IntuitionBase
 {
 struct Library LibNode ;

 struct View ViewLord ;

 struct Window * ActiveWindow ;
 struct Screen * ActiveScreen ;




 struct Screen * FirstScreen ;

 ULONG Flags ;
 WORD MouseY , MouseX ;


 ULONG Seconds ;
 ULONG Micros ;





 } ;
#line 19 "intuition\classusr.h"
 typedef ULONG Object ;

 typedef STRPTR ClassID ;







 typedef struct {
 ULONG MethodID ;

 } * Msg ;












































 struct opSet {
 ULONG MethodID ;
 struct TagItem * ops_AttrList ;
 struct GadgetInfo * ops_GInfo ;



 } ;


 struct opUpdate {
 ULONG MethodID ;
 struct TagItem * opu_AttrList ;
 struct GadgetInfo * opu_GInfo ;



 ULONG opu_Flags ;
 } ;











 struct opGet {
 ULONG MethodID ;
 ULONG opg_AttrID ;
 ULONG * opg_Storage ;


 } ;


 struct opAddTail {
 ULONG MethodID ;
 struct List * opat_List ;
 } ;



 struct opMember {
 ULONG MethodID ;
 Object * opam_Object ;
 } ;
#line 31 "intuition\classes.h"
 typedef struct IClass
 {
 struct Hook cl_Dispatcher ;
 ULONG cl_Reserved ;
 struct IClass * cl_Super ;
 ClassID cl_ID ;

 UWORD cl_InstOffset ;
 UWORD cl_InstSize ;

 ULONG cl_UserData ;
 ULONG cl_SubclassCount ;
 ULONG cl_ObjectCount ;
 ULONG cl_Flags ;

 } Class ;



























 struct _Object
 {
 struct MinNode o_Node ;
 struct IClass * o_Class ;

 } ;




















 struct ClassLibrary
 {
 struct Library cl_Lib ;
 UWORD cl_Pad ;
 Class * cl_Class ;

 } ;
#line 24 "intuition\cghooks.h"
 struct GadgetInfo {

 struct Screen * gi_Screen ;
 struct Window * gi_Window ;
 struct Requester * gi_Requester ;





 struct RastPort * gi_RastPort ;
 struct Layer * gi_Layer ;










 struct IBox gi_Domain ;


 struct {
 UBYTE DetailPen ;
 UBYTE BlockPen ;
 } gi_Pens ;





 struct DrawInfo * gi_DrInfo ;




 ULONG gi_Reserved [ 6 ] ;
 } ;



 struct PGX {
 struct IBox pgx_Container ;
 struct IBox pgx_NewKnob ;
 } ;
#line 31 "clib\intuition_protos.h"
 void OpenIntuition ( void ) ;
 void Intuition ( struct InputEvent * iEvent ) ;
 UWORD AddGadget ( struct Window * window , struct Gadget * gadget , ULONG position ) ;
 BOOL ClearDMRequest ( struct Window * window ) ;
 void ClearMenuStrip ( struct Window * window ) ;
 void ClearPointer ( struct Window * window ) ;
 BOOL CloseScreen ( struct Screen * screen ) ;
 void CloseWindow ( struct Window * window ) ;
 LONG CloseWorkBench ( void ) ;
 void CurrentTime ( ULONG * seconds , ULONG * micros ) ;
 BOOL DisplayAlert ( ULONG alertNumber , CONST_STRPTR string , ULONG height ) ;
 void DisplayBeep ( struct Screen * screen ) ;
 BOOL DoubleClick ( ULONG sSeconds , ULONG sMicros , ULONG cSeconds , ULONG cMicros ) ;
 void DrawBorder ( struct RastPort * rp , const struct Border * border , LONG leftOffset , LONG topOffset ) ;
 void DrawImage ( struct RastPort * rp , const struct Image * image , LONG leftOffset , LONG topOffset ) ;
 void EndRequest ( struct Requester * requester , struct Window * window ) ;
 struct Preferences * GetDefPrefs ( struct Preferences * preferences , LONG size ) ;
 struct Preferences * GetPrefs ( struct Preferences * preferences , LONG size ) ;
 void InitRequester ( struct Requester * requester ) ;
 struct MenuItem * ItemAddress ( const struct Menu * menuStrip , ULONG menuNumber ) ;
 BOOL ModifyIDCMP ( struct Window * window , ULONG flags ) ;
 void ModifyProp ( struct Gadget * gadget , struct Window * window , struct Requester * requester , ULONG flags , ULONG horizPot , ULONG vertPot , ULONG horizBody , ULONG vertBody ) ;
 void MoveScreen ( struct Screen * screen , LONG dx , LONG dy ) ;
 void MoveWindow ( struct Window * window , LONG dx , LONG dy ) ;
 void OffGadget ( struct Gadget * gadget , struct Window * window , struct Requester * requester ) ;
 void OffMenu ( struct Window * window , ULONG menuNumber ) ;
 void OnGadget ( struct Gadget * gadget , struct Window * window , struct Requester * requester ) ;
 void OnMenu ( struct Window * window , ULONG menuNumber ) ;
 struct Screen * OpenScreen ( const struct NewScreen * newScreen ) ;
 struct Window * OpenWindow ( const struct NewWindow * newWindow ) ;
 ULONG OpenWorkBench ( void ) ;
 void PrintIText ( struct RastPort * rp , const struct IntuiText * iText , LONG left , LONG top ) ;
 void RefreshGadgets ( struct Gadget * gadgets , struct Window * window , struct Requester * requester ) ;
 UWORD RemoveGadget ( struct Window * window , struct Gadget * gadget ) ;



 void ReportMouse ( LONG flag , struct Window * window ) ;
 void ReportMouse1 ( struct Window * window , LONG flag ) ;
 BOOL Request ( struct Requester * requester , struct Window * window ) ;
 void ScreenToBack ( struct Screen * screen ) ;
 void ScreenToFront ( struct Screen * screen ) ;
 BOOL SetDMRequest ( struct Window * window , struct Requester * requester ) ;
 BOOL SetMenuStrip ( struct Window * window , struct Menu * menu ) ;
 void SetPointer ( struct Window * window , UWORD * pointer , LONG height , LONG width , LONG xOffset , LONG yOffset ) ;
 void SetWindowTitles ( struct Window * window , CONST_STRPTR windowTitle , CONST_STRPTR screenTitle ) ;
 void ShowTitle ( struct Screen * screen , LONG showIt ) ;
 void SizeWindow ( struct Window * window , LONG dx , LONG dy ) ;
 struct View * ViewAddress ( void ) ;
 struct ViewPort * ViewPortAddress ( const struct Window * window ) ;
 void WindowToBack ( struct Window * window ) ;
 void WindowToFront ( struct Window * window ) ;
 BOOL WindowLimits ( struct Window * window , LONG widthMin , LONG heightMin , ULONG widthMax , ULONG heightMax ) ;

 struct Preferences * SetPrefs ( const struct Preferences * preferences , LONG size , LONG inform ) ;

 LONG IntuiTextLength ( const struct IntuiText * iText ) ;
 BOOL WBenchToBack ( void ) ;
 BOOL WBenchToFront ( void ) ;

 BOOL AutoRequest ( struct Window * window , const struct IntuiText * body , const struct IntuiText * posText , const struct IntuiText * negText , ULONG pFlag , ULONG nFlag , ULONG width , ULONG height ) ;
 void BeginRefresh ( struct Window * window ) ;
 struct Window * BuildSysRequest ( struct Window * window , const struct IntuiText * body , const struct IntuiText * posText , const struct IntuiText * negText , ULONG flags , ULONG width , ULONG height ) ;
 void EndRefresh ( struct Window * window , LONG complete ) ;
 void FreeSysRequest ( struct Window * window ) ;



 LONG MakeScreen ( struct Screen * screen ) ;
 LONG RemakeDisplay ( void ) ;
 LONG RethinkDisplay ( void ) ;

 APTR AllocRemember ( struct Remember * * rememberKey , ULONG size , ULONG flags ) ;

 void AlohaWorkbench ( LONG wbport ) ;
 void FreeRemember ( struct Remember * * rememberKey , LONG reallyForget ) ;

 ULONG LockIBase ( ULONG dontknow ) ;
 void UnlockIBase ( ULONG ibLock ) ;

 LONG GetScreenData ( APTR buffer , ULONG size , ULONG type , const struct Screen * screen ) ;
 void RefreshGList ( struct Gadget * gadgets , struct Window * window , struct Requester * requester , LONG numGad ) ;
 UWORD AddGList ( struct Window * window , struct Gadget * gadget , ULONG position , LONG numGad , struct Requester * requester ) ;
 UWORD RemoveGList ( struct Window * remPtr , struct Gadget * gadget , LONG numGad ) ;
 void ActivateWindow ( struct Window * window ) ;
 void RefreshWindowFrame ( struct Window * window ) ;
 BOOL ActivateGadget ( struct Gadget * gadgets , struct Window * window , struct Requester * requester ) ;
 void NewModifyProp ( struct Gadget * gadget , struct Window * window , struct Requester * requester , ULONG flags , ULONG horizPot , ULONG vertPot , ULONG horizBody , ULONG vertBody , LONG numGad ) ;

 LONG QueryOverscan ( ULONG displayID , struct Rectangle * rect , LONG oScanType ) ;
 void MoveWindowInFrontOf ( struct Window * window , struct Window * behindWindow ) ;
 void ChangeWindowBox ( struct Window * window , LONG left , LONG top , LONG width , LONG height ) ;
 struct Hook * SetEditHook ( struct Hook * hook ) ;
 LONG SetMouseQueue ( struct Window * window , ULONG queueLength ) ;
 void ZipWindow ( struct Window * window ) ;

 struct Screen * LockPubScreen ( CONST_STRPTR name ) ;
 void UnlockPubScreen ( CONST_STRPTR name , struct Screen * screen ) ;
 struct List * LockPubScreenList ( void ) ;
 void UnlockPubScreenList ( void ) ;
 STRPTR NextPubScreen ( const struct Screen * screen , STRPTR namebuf ) ;
 void SetDefaultPubScreen ( CONST_STRPTR name ) ;
 UWORD SetPubScreenModes ( ULONG modes ) ;
 UWORD PubScreenStatus ( struct Screen * screen , ULONG statusFlags ) ;

 struct RastPort * ObtainGIRPort ( struct GadgetInfo * gInfo ) ;
 void ReleaseGIRPort ( struct RastPort * rp ) ;
 void GadgetMouse ( struct Gadget * gadget , struct GadgetInfo * gInfo , WORD * mousePoint ) ;
 void GetDefaultPubScreen ( STRPTR nameBuffer ) ;
 LONG EasyRequestArgs ( struct Window * window , const struct EasyStruct * easyStruct , ULONG * idcmpPtr , CONST_APTR args ) ;
 LONG EasyRequest ( struct Window * window , const struct EasyStruct * easyStruct , ULONG * idcmpPtr , ... ) ;
 struct Window * BuildEasyRequestArgs ( struct Window * window , const struct EasyStruct * easyStruct , ULONG idcmp , CONST_APTR args ) ;
 struct Window * BuildEasyRequest ( struct Window * window , const struct EasyStruct * easyStruct , ULONG idcmp , ... ) ;
 LONG SysReqHandler ( struct Window * window , ULONG * idcmpPtr , LONG waitInput ) ;
 struct Window * OpenWindowTagList ( const struct NewWindow * newWindow , const struct TagItem * tagList ) ;
 struct Window * OpenWindowTags ( const struct NewWindow * newWindow , ULONG tag1Type , ... ) ;
 struct Screen * OpenScreenTagList ( const struct NewScreen * newScreen , const struct TagItem * tagList ) ;
 struct Screen * OpenScreenTags ( const struct NewScreen * newScreen , ULONG tag1Type , ... ) ;


 void DrawImageState ( struct RastPort * rp , const struct Image * image , LONG leftOffset , LONG topOffset , ULONG state , struct DrawInfo * drawInfo ) ;
 BOOL PointInImage ( ULONG point , const struct Image * image ) ;
 void EraseImage ( struct RastPort * rp , const struct Image * image , LONG leftOffset , LONG topOffset ) ;

 APTR NewObjectA ( struct IClass * classPtr , CONST_STRPTR classID , const struct TagItem * tagList ) ;
 APTR NewObject ( struct IClass * classPtr , CONST_STRPTR classID , ULONG tag1 , ... ) ;

 void DisposeObject ( APTR object ) ;
 ULONG SetAttrsA ( APTR object , const struct TagItem * tagList ) ;
 ULONG SetAttrs ( APTR object , ULONG tag1 , ... ) ;

 ULONG GetAttr ( ULONG attrID , APTR object , ULONG * storagePtr ) ;


 ULONG SetGadgetAttrsA ( struct Gadget * gadget , struct Window * window , struct Requester * requester , const struct TagItem * tagList ) ;
 ULONG SetGadgetAttrs ( struct Gadget * gadget , struct Window * window , struct Requester * requester , ULONG tag1 , ... ) ;


 APTR NextObject ( CONST_APTR objectPtrPtr ) ;
 struct IClass * MakeClass ( CONST_STRPTR classID , CONST_STRPTR superClassID , const struct IClass * superClassPtr , ULONG instanceSize , ULONG flags ) ;
 void AddClass ( struct IClass * classPtr ) ;


 struct DrawInfo * GetScreenDrawInfo ( struct Screen * screen ) ;
 void FreeScreenDrawInfo ( struct Screen * screen , struct DrawInfo * drawInfo ) ;

 BOOL ResetMenuStrip ( struct Window * window , struct Menu * menu ) ;
 void RemoveClass ( struct IClass * classPtr ) ;
 BOOL FreeClass ( struct IClass * classPtr ) ;

 struct ScreenBuffer * AllocScreenBuffer ( struct Screen * sc , struct BitMap * bm , ULONG flags ) ;
 void FreeScreenBuffer ( struct Screen * sc , struct ScreenBuffer * sb ) ;
 ULONG ChangeScreenBuffer ( struct Screen * sc , struct ScreenBuffer * sb ) ;
 void ScreenDepth ( struct Screen * screen , ULONG flags , APTR reserved ) ;
 void ScreenPosition ( struct Screen * screen , ULONG flags , LONG x1 , LONG y1 , LONG x2 , LONG y2 ) ;
 void ScrollWindowRaster ( struct Window * win , LONG dx , LONG dy , LONG xMin , LONG yMin , LONG xMax , LONG yMax ) ;
 void LendMenus ( struct Window * fromwindow , struct Window * towindow ) ;
 ULONG DoGadgetMethodA ( struct Gadget * gad , struct Window * win , struct Requester * req , Msg message ) ;
 ULONG DoGadgetMethod ( struct Gadget * gad , struct Window * win , struct Requester * req , ULONG methodID , ... ) ;
 void SetWindowPointerA ( struct Window * win , const struct TagItem * taglist ) ;
 void SetWindowPointer ( struct Window * win , ULONG tag1 , ... ) ;
 BOOL TimedDisplayAlert ( ULONG alertNumber , CONST_STRPTR string , ULONG height , ULONG time ) ;
 void HelpControl ( struct Window * win , ULONG flags ) ;

 BOOL ShowWindow ( struct Window * window , struct Window * other ) ;
 BOOL HideWindow ( struct Window * window ) ;

 ULONG IntuitionControlA ( APTR object , const struct TagItem * taglist ) ;
 ULONG IntuitionControl ( APTR object , ... ) ;
#line 10 "proto\intuition.h"
#pragma stdargs-off



 extern struct IntuitionBase * IntuitionBase ;
#line 8 "inline\intuition_protos.h"
 void __OpenIntuition ( __reg ( "a6" ) void * ) = "\tjsr\t-30(a6)" ;


 void __Intuition ( __reg ( "a6" ) void * , __reg ( "a0" ) struct InputEvent * iEvent ) = "\tjsr\t-36(a6)" ;


 UWORD __AddGadget ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Gadget * gadget , __reg ( "d0" ) ULONG position ) = "\tjsr\t-42(a6)" ;


 BOOL __ClearDMRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-48(a6)" ;


 void __ClearMenuStrip ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-54(a6)" ;


 void __ClearPointer ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-60(a6)" ;


 BOOL __CloseScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen ) = "\tjsr\t-66(a6)" ;


 void __CloseWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-72(a6)" ;


 LONG __CloseWorkBench ( __reg ( "a6" ) void * ) = "\tjsr\t-78(a6)" ;


 void __CurrentTime ( __reg ( "a6" ) void * , __reg ( "a0" ) ULONG * seconds , __reg ( "a1" ) ULONG * micros ) = "\tjsr\t-84(a6)" ;


 BOOL __DisplayAlert ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG alertNumber , __reg ( "a0" ) CONST_STRPTR string , __reg ( "d1" ) ULONG height ) = "\tjsr\t-90(a6)" ;


 void __DisplayBeep ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen ) = "\tjsr\t-96(a6)" ;


 BOOL __DoubleClick ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG sSeconds , __reg ( "d1" ) ULONG sMicros , __reg ( "d2" ) ULONG cSeconds , __reg ( "d3" ) ULONG cMicros ) = "\tjsr\t-102(a6)" ;


 void __DrawBorder ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rp , __reg ( "a1" ) const struct Border * border , __reg ( "d0" ) LONG leftOffset , __reg ( "d1" ) LONG topOffset ) = "\tjsr\t-108(a6)" ;


 void __DrawImage ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rp , __reg ( "a1" ) const struct Image * image , __reg ( "d0" ) LONG leftOffset , __reg ( "d1" ) LONG topOffset ) = "\tjsr\t-114(a6)" ;


 void __EndRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Requester * requester , __reg ( "a1" ) struct Window * window ) = "\tjsr\t-120(a6)" ;


 struct Preferences * __GetDefPrefs ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Preferences * preferences , __reg ( "d0" ) LONG size ) = "\tjsr\t-126(a6)" ;


 struct Preferences * __GetPrefs ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Preferences * preferences , __reg ( "d0" ) LONG size ) = "\tjsr\t-132(a6)" ;


 void __InitRequester ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Requester * requester ) = "\tjsr\t-138(a6)" ;


 struct MenuItem * __ItemAddress ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct Menu * menuStrip , __reg ( "d0" ) ULONG menuNumber ) = "\tjsr\t-144(a6)" ;


 BOOL __ModifyIDCMP ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) ULONG flags ) = "\tjsr\t-150(a6)" ;


 void __ModifyProp ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadget , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester , __reg ( "d0" ) ULONG flags , __reg ( "d1" ) ULONG horizPot , __reg ( "d2" ) ULONG vertPot , __reg ( "d3" ) ULONG horizBody , __reg ( "d4" ) ULONG vertBody ) = "\tjsr\t-156(a6)" ;


 void __MoveScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , __reg ( "d0" ) LONG dx , __reg ( "d1" ) LONG dy ) = "\tjsr\t-162(a6)" ;


 void __MoveWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) LONG dx , __reg ( "d1" ) LONG dy ) = "\tjsr\t-168(a6)" ;


 void __OffGadget ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadget , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester ) = "\tjsr\t-174(a6)" ;


 void __OffMenu ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) ULONG menuNumber ) = "\tjsr\t-180(a6)" ;


 void __OnGadget ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadget , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester ) = "\tjsr\t-186(a6)" ;


 void __OnMenu ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) ULONG menuNumber ) = "\tjsr\t-192(a6)" ;


 struct Screen * __OpenScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewScreen * newScreen ) = "\tjsr\t-198(a6)" ;


 struct Window * __OpenWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewWindow * newWindow ) = "\tjsr\t-204(a6)" ;


 ULONG __OpenWorkBench ( __reg ( "a6" ) void * ) = "\tjsr\t-210(a6)" ;


 void __PrintIText ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rp , __reg ( "a1" ) const struct IntuiText * iText , __reg ( "d0" ) LONG left , __reg ( "d1" ) LONG top ) = "\tjsr\t-216(a6)" ;


 void __RefreshGadgets ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadgets , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester ) = "\tjsr\t-222(a6)" ;


 UWORD __RemoveGadget ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Gadget * gadget ) = "\tjsr\t-228(a6)" ;


 void __ReportMouse ( __reg ( "a6" ) void * , __reg ( "d0" ) LONG flag , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-234(a6)" ;


 void __ReportMouse1 ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) LONG flag ) = "\tjsr\t-234(a6)" ;


 BOOL __Request ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Requester * requester , __reg ( "a1" ) struct Window * window ) = "\tjsr\t-240(a6)" ;


 void __ScreenToBack ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen ) = "\tjsr\t-246(a6)" ;


 void __ScreenToFront ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen ) = "\tjsr\t-252(a6)" ;


 BOOL __SetDMRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Requester * requester ) = "\tjsr\t-258(a6)" ;


 BOOL __SetMenuStrip ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Menu * menu ) = "\tjsr\t-264(a6)" ;


 void __SetPointer ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) UWORD * pointer , __reg ( "d0" ) LONG height , __reg ( "d1" ) LONG width , __reg ( "d2" ) LONG xOffset , __reg ( "d3" ) LONG yOffset ) = "\tjsr\t-270(a6)" ;


 void __SetWindowTitles ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) CONST_STRPTR windowTitle , __reg ( "a2" ) CONST_STRPTR screenTitle ) = "\tjsr\t-276(a6)" ;


 void __ShowTitle ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , __reg ( "d0" ) LONG showIt ) = "\tjsr\t-282(a6)" ;


 void __SizeWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) LONG dx , __reg ( "d1" ) LONG dy ) = "\tjsr\t-288(a6)" ;


 struct View * __ViewAddress ( __reg ( "a6" ) void * ) = "\tjsr\t-294(a6)" ;


 struct ViewPort * __ViewPortAddress ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct Window * window ) = "\tjsr\t-300(a6)" ;


 void __WindowToBack ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-306(a6)" ;


 void __WindowToFront ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-312(a6)" ;


 BOOL __WindowLimits ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) LONG widthMin , __reg ( "d1" ) LONG heightMin , __reg ( "d2" ) ULONG widthMax , __reg ( "d3" ) ULONG heightMax ) = "\tjsr\t-318(a6)" ;


 struct Preferences * __SetPrefs ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct Preferences * preferences , __reg ( "d0" ) LONG size , __reg ( "d1" ) LONG inform ) = "\tjsr\t-324(a6)" ;


 LONG __IntuiTextLength ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct IntuiText * iText ) = "\tjsr\t-330(a6)" ;


 BOOL __WBenchToBack ( __reg ( "a6" ) void * ) = "\tjsr\t-336(a6)" ;


 BOOL __WBenchToFront ( __reg ( "a6" ) void * ) = "\tjsr\t-342(a6)" ;


 BOOL __AutoRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) const struct IntuiText * body , __reg ( "a2" ) const struct IntuiText * posText , __reg ( "a3" ) const struct IntuiText * negText , __reg ( "d0" ) ULONG pFlag , __reg ( "d1" ) ULONG nFlag , __reg ( "d2" ) ULONG width , __reg ( "d3" ) ULONG height ) = "\tjsr\t-348(a6)" ;


 void __BeginRefresh ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-354(a6)" ;


 struct Window * __BuildSysRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) const struct IntuiText * body , __reg ( "a2" ) const struct IntuiText * posText , __reg ( "a3" ) const struct IntuiText * negText , __reg ( "d0" ) ULONG flags , __reg ( "d1" ) ULONG width , __reg ( "d2" ) ULONG height ) = "\tjsr\t-360(a6)" ;


 void __EndRefresh ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) LONG complete ) = "\tjsr\t-366(a6)" ;


 void __FreeSysRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-372(a6)" ;


 LONG __MakeScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen ) = "\tjsr\t-378(a6)" ;


 LONG __RemakeDisplay ( __reg ( "a6" ) void * ) = "\tjsr\t-384(a6)" ;


 LONG __RethinkDisplay ( __reg ( "a6" ) void * ) = "\tjsr\t-390(a6)" ;


 APTR __AllocRemember ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Remember * * rememberKey , __reg ( "d0" ) ULONG size , __reg ( "d1" ) ULONG flags ) = "\tjsr\t-396(a6)" ;


 void __AlohaWorkbench ( __reg ( "a6" ) void * , __reg ( "a0" ) void * wbport ) = "\tjsr\t-402(a6)" ;


 void __FreeRemember ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Remember * * rememberKey , __reg ( "d0" ) LONG reallyForget ) = "\tjsr\t-408(a6)" ;


 ULONG __LockIBase ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG dontknow ) = "\tjsr\t-414(a6)" ;


 void __UnlockIBase ( __reg ( "a6" ) void * , __reg ( "a0" ) void * ibLock ) = "\tjsr\t-420(a6)" ;


 LONG __GetScreenData ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR buffer , __reg ( "d0" ) ULONG size , __reg ( "d1" ) ULONG type , __reg ( "a1" ) const struct Screen * screen ) = "\tjsr\t-426(a6)" ;


 void __RefreshGList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadgets , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester , __reg ( "d0" ) LONG numGad ) = "\tjsr\t-432(a6)" ;


 UWORD __AddGList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Gadget * gadget , __reg ( "d0" ) ULONG position , __reg ( "d1" ) LONG numGad , __reg ( "a2" ) struct Requester * requester ) = "\tjsr\t-438(a6)" ;


 UWORD __RemoveGList ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * remPtr , __reg ( "a1" ) struct Gadget * gadget , __reg ( "d0" ) LONG numGad ) = "\tjsr\t-444(a6)" ;


 void __ActivateWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-450(a6)" ;


 void __RefreshWindowFrame ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-456(a6)" ;


 BOOL __ActivateGadget ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadgets , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester ) = "\tjsr\t-462(a6)" ;


 void __NewModifyProp ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadget , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester , __reg ( "d0" ) ULONG flags , __reg ( "d1" ) ULONG horizPot , __reg ( "d2" ) ULONG vertPot , __reg ( "d3" ) ULONG horizBody , __reg ( "d4" ) ULONG vertBody , __reg ( "d5" ) LONG numGad ) = "\tjsr\t-468(a6)" ;


 LONG __QueryOverscan ( __reg ( "a6" ) void * , __reg ( "a0" ) void * displayID , __reg ( "a1" ) struct Rectangle * rect , __reg ( "d0" ) LONG oScanType ) = "\tjsr\t-474(a6)" ;


 void __MoveWindowInFrontOf ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Window * behindWindow ) = "\tjsr\t-480(a6)" ;


 void __ChangeWindowBox ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) LONG left , __reg ( "d1" ) LONG top , __reg ( "d2" ) LONG width , __reg ( "d3" ) LONG height ) = "\tjsr\t-486(a6)" ;


 struct Hook * __SetEditHook ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Hook * hook ) = "\tjsr\t-492(a6)" ;


 LONG __SetMouseQueue ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "d0" ) ULONG queueLength ) = "\tjsr\t-498(a6)" ;


 void __ZipWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-504(a6)" ;


 struct Screen * __LockPubScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_STRPTR name ) = "\tjsr\t-510(a6)" ;


 void __UnlockPubScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_STRPTR name , __reg ( "a1" ) struct Screen * screen ) = "\tjsr\t-516(a6)" ;


 struct List * __LockPubScreenList ( __reg ( "a6" ) void * ) = "\tjsr\t-522(a6)" ;


 void __UnlockPubScreenList ( __reg ( "a6" ) void * ) = "\tjsr\t-528(a6)" ;


 STRPTR __NextPubScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct Screen * screen , __reg ( "a1" ) STRPTR namebuf ) = "\tjsr\t-534(a6)" ;


 void __SetDefaultPubScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_STRPTR name ) = "\tjsr\t-540(a6)" ;


 UWORD __SetPubScreenModes ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG modes ) = "\tjsr\t-546(a6)" ;


 UWORD __PubScreenStatus ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , __reg ( "d0" ) ULONG statusFlags ) = "\tjsr\t-552(a6)" ;


 struct RastPort * __ObtainGIRPort ( __reg ( "a6" ) void * , __reg ( "a0" ) struct GadgetInfo * gInfo ) = "\tjsr\t-558(a6)" ;


 void __ReleaseGIRPort ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rp ) = "\tjsr\t-564(a6)" ;


 void __GadgetMouse ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadget , __reg ( "a1" ) struct GadgetInfo * gInfo , __reg ( "a2" ) WORD * mousePoint ) = "\tjsr\t-570(a6)" ;


 void __GetDefaultPubScreen ( __reg ( "a6" ) void * , __reg ( "a0" ) STRPTR nameBuffer ) = "\tjsr\t-582(a6)" ;


 LONG __EasyRequestArgs ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) const struct EasyStruct * easyStruct , __reg ( "a2" ) ULONG * idcmpPtr , __reg ( "a3" ) CONST_APTR args ) = "\tjsr\t-588(a6)" ;



 LONG __EasyRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) const struct EasyStruct * easyStruct , __reg ( "a2" ) ULONG * idcmpPtr , ... ) = "\tmove.l\ta3,-(a7)\n\tlea\t4(a7),a3\n\tjsr\t-588(a6)\n\tmovea.l\t(a7)+,a3" ;



 struct Window * __BuildEasyRequestArgs ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) const struct EasyStruct * easyStruct , __reg ( "d0" ) ULONG idcmp , __reg ( "a3" ) CONST_APTR args ) = "\tjsr\t-594(a6)" ;



 struct Window * __BuildEasyRequest ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) const struct EasyStruct * easyStruct , __reg ( "d0" ) ULONG idcmp , ... ) = "\tmove.l\ta3,-(a7)\n\tlea\t4(a7),a3\n\tjsr\t-594(a6)\n\tmovea.l\t(a7)+,a3" ;



 LONG __SysReqHandler ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) ULONG * idcmpPtr , __reg ( "d0" ) LONG waitInput ) = "\tjsr\t-600(a6)" ;


 struct Window * __OpenWindowTagList ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewWindow * newWindow , __reg ( "a1" ) const struct TagItem * tagList ) = "\tjsr\t-606(a6)" ;



 struct Window * __OpenWindowTags ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewWindow * newWindow , ULONG tagList , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-606(a6)\n\tmovea.l\t(a7)+,a1" ;



 struct Screen * __OpenScreenTagList ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewScreen * newScreen , __reg ( "a1" ) const struct TagItem * tagList ) = "\tjsr\t-612(a6)" ;



 struct Screen * __OpenScreenTags ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewScreen * newScreen , ULONG tagList , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-612(a6)\n\tmovea.l\t(a7)+,a1" ;



 void __DrawImageState ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rp , __reg ( "a1" ) const struct Image * image , __reg ( "d0" ) LONG leftOffset , __reg ( "d1" ) LONG topOffset , __reg ( "d2" ) ULONG state , __reg ( "a2" ) struct DrawInfo * drawInfo ) = "\tjsr\t-618(a6)" ;


 BOOL __PointInImage ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG point , __reg ( "a0" ) const struct Image * image ) = "\tjsr\t-624(a6)" ;


 void __EraseImage ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rp , __reg ( "a1" ) const struct Image * image , __reg ( "d0" ) LONG leftOffset , __reg ( "d1" ) LONG topOffset ) = "\tjsr\t-630(a6)" ;


 APTR __NewObjectA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct IClass * classPtr , __reg ( "a1" ) CONST_STRPTR classID , __reg ( "a2" ) const struct TagItem * tagList ) = "\tjsr\t-636(a6)" ;


 void __DisposeObject ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR object ) = "\tjsr\t-642(a6)" ;


 ULONG __SetAttrsA ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR object , __reg ( "a1" ) const struct TagItem * tagList ) = "\tjsr\t-648(a6)" ;



 ULONG __SetAttrs ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR object , ULONG tagList , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-648(a6)\n\tmovea.l\t(a7)+,a1" ;



 ULONG __GetAttr ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG attrID , __reg ( "a0" ) APTR object , __reg ( "a1" ) ULONG * storagePtr ) = "\tjsr\t-654(a6)" ;


 ULONG __SetGadgetAttrsA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadget , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester , __reg ( "a3" ) const struct TagItem * tagList ) = "\tjsr\t-660(a6)" ;



 ULONG __SetGadgetAttrs ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gadget , __reg ( "a1" ) struct Window * window , __reg ( "a2" ) struct Requester * requester , ULONG tagList , ... ) = "\tmove.l\ta3,-(a7)\n\tlea\t4(a7),a3\n\tjsr\t-660(a6)\n\tmovea.l\t(a7)+,a3" ;



 APTR __NextObject ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_APTR objectPtrPtr ) = "\tjsr\t-666(a6)" ;


 struct IClass * __MakeClass ( __reg ( "a6" ) void * , __reg ( "a0" ) CONST_STRPTR classID , __reg ( "a1" ) CONST_STRPTR superClassID , __reg ( "a2" ) const struct IClass * superClassPtr , __reg ( "d0" ) ULONG instanceSize , __reg ( "d1" ) ULONG flags ) = "\tjsr\t-678(a6)" ;


 void __AddClass ( __reg ( "a6" ) void * , __reg ( "a0" ) struct IClass * classPtr ) = "\tjsr\t-684(a6)" ;


 struct DrawInfo * __GetScreenDrawInfo ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen ) = "\tjsr\t-690(a6)" ;


 void __FreeScreenDrawInfo ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , __reg ( "a1" ) struct DrawInfo * drawInfo ) = "\tjsr\t-696(a6)" ;


 BOOL __ResetMenuStrip ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Menu * menu ) = "\tjsr\t-702(a6)" ;


 void __RemoveClass ( __reg ( "a6" ) void * , __reg ( "a0" ) struct IClass * classPtr ) = "\tjsr\t-708(a6)" ;


 BOOL __FreeClass ( __reg ( "a6" ) void * , __reg ( "a0" ) struct IClass * classPtr ) = "\tjsr\t-714(a6)" ;


 struct ScreenBuffer * __AllocScreenBuffer ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * sc , __reg ( "a1" ) struct BitMap * bm , __reg ( "d0" ) ULONG flags ) = "\tjsr\t-768(a6)" ;


 void __FreeScreenBuffer ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * sc , __reg ( "a1" ) struct ScreenBuffer * sb ) = "\tjsr\t-774(a6)" ;


 ULONG __ChangeScreenBuffer ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * sc , __reg ( "a1" ) struct ScreenBuffer * sb ) = "\tjsr\t-780(a6)" ;


 void __ScreenDepth ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , __reg ( "d0" ) ULONG flags , __reg ( "a1" ) APTR reserved ) = "\tjsr\t-786(a6)" ;


 void __ScreenPosition ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , __reg ( "d0" ) ULONG flags , __reg ( "d1" ) LONG x1 , __reg ( "d2" ) LONG y1 , __reg ( "d3" ) LONG x2 , __reg ( "d4" ) LONG y2 ) = "\tjsr\t-792(a6)" ;


 void __ScrollWindowRaster ( __reg ( "a6" ) void * , __reg ( "a1" ) struct Window * win , __reg ( "d0" ) LONG dx , __reg ( "d1" ) LONG dy , __reg ( "d2" ) LONG xMin , __reg ( "d3" ) LONG yMin , __reg ( "d4" ) LONG xMax , __reg ( "d5" ) LONG yMax ) = "\tjsr\t-798(a6)" ;


 void __LendMenus ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * fromwindow , __reg ( "a1" ) struct Window * towindow ) = "\tjsr\t-804(a6)" ;


 ULONG __DoGadgetMethodA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct Window * win , __reg ( "a2" ) struct Requester * req , __reg ( "a3" ) Msg message ) = "\tjsr\t-810(a6)" ;



 ULONG __DoGadgetMethod ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct Window * win , __reg ( "a2" ) struct Requester * req , ULONG message , ... ) = "\tmove.l\ta3,-(a7)\n\tlea\t4(a7),a3\n\tjsr\t-810(a6)\n\tmovea.l\t(a7)+,a3" ;



 void __SetWindowPointerA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * win , __reg ( "a1" ) const struct TagItem * taglist ) = "\tjsr\t-816(a6)" ;



 void __SetWindowPointer ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * win , ULONG taglist , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-816(a6)\n\tmovea.l\t(a7)+,a1" ;



 BOOL __TimedDisplayAlert ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG alertNumber , __reg ( "a0" ) CONST_STRPTR string , __reg ( "d1" ) ULONG height , __reg ( "a1" ) void * time ) = "\tjsr\t-822(a6)" ;


 void __HelpControl ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * win , __reg ( "d0" ) ULONG flags ) = "\tjsr\t-828(a6)" ;


 BOOL __ShowWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window , __reg ( "a1" ) struct Window * other ) = "\tjsr\t-834(a6)" ;


 BOOL __HideWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * window ) = "\tjsr\t-840(a6)" ;


 ULONG __IntuitionControlA ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR object , __reg ( "a1" ) const struct TagItem * taglist ) = "\tjsr\t-1212(a6)" ;



 ULONG __IntuitionControl ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR object , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-1212(a6)\n\tmovea.l\t(a7)+,a1" ;
#line 8 "proto\gadtools.h"
#pragma stdargs-on
#line 379 "intuition\gadgetclass.h"
 struct gpHitTest
 {
 ULONG MethodID ;
 struct GadgetInfo * gpht_GInfo ;
 struct
 {
 WORD X ;
 WORD Y ;
 } gpht_Mouse ;
 } ;























 struct gpRender
 {
 ULONG MethodID ;
 struct GadgetInfo * gpr_GInfo ;
 struct RastPort * gpr_RPort ;
 LONG gpr_Redraw ;
 } ;









 struct gpInput
 {
 ULONG MethodID ;
 struct GadgetInfo * gpi_GInfo ;
 struct InputEvent * gpi_IEvent ;
 LONG * gpi_Termination ;
 struct
 {
 WORD X ;
 WORD Y ;
 } gpi_Mouse ;








 struct TabletData * gpi_TabletData ;
 } ;






















 struct gpGoInactive
 {
 ULONG MethodID ;
 struct GadgetInfo * gpgi_GInfo ;


 ULONG gpgi_Abort ;



 } ;













 struct gpLayout
 {
 ULONG MethodID ;
 struct GadgetInfo * gpl_GInfo ;
 ULONG gpl_Initial ;



 } ;







 struct gpDomain
 {
 ULONG MethodID ;
 struct GadgetInfo * gpd_GInfo ;
 struct RastPort * gpd_RPort ;
 LONG gpd_Which ;
 struct IBox gpd_Domain ;
 struct TagItem * gpd_Attrs ;
 } ;


















 struct gpKeyTest
 {
 ULONG MethodID ;
 struct GadgetInfo * gpkt_GInfo ;
 struct IntuiMessage * gpkt_IMsg ;
 ULONG gpkt_VanillaKey ;
 } ;












 struct gpKeyInput
 {
 ULONG MethodID ;
 struct GadgetInfo * gpk_GInfo ;
 struct InputEvent * gpk_IEvent ;
 LONG * gpk_Termination ;
 } ;









 struct gpKeyGoInactive
 {
 ULONG MethodID ;
 struct GadgetInfo * gpki_GInfo ;
 ULONG gpki_Abort ;
 } ;
#line 79 "libraries\gadtools.h"
 struct NewGadget
 {
 WORD ng_LeftEdge , ng_TopEdge ;
 WORD ng_Width , ng_Height ;
 CONST_STRPTR ng_GadgetText ;
 const struct TextAttr * ng_TextAttr ;
 UWORD ng_GadgetID ;
 ULONG ng_Flags ;
 APTR ng_VisualInfo ;
 APTR ng_UserData ;
 } ;























 struct NewMenu
 {
 UBYTE nm_Type ;

 CONST_STRPTR nm_Label ;
 CONST_STRPTR nm_CommKey ;
 UWORD nm_Flags ;
 LONG nm_MutualExclude ;
 APTR nm_UserData ;
 } ;




































































































































































































































































































































































































 struct LVDrawMsg
 {
 ULONG lvdm_MethodID ;
 struct RastPort * lvdm_RastPort ;
 struct DrawInfo * lvdm_DrawInfo ;
 struct Rectangle lvdm_Bounds ;
 ULONG lvdm_State ;
 } ;
#line 34 "clib\gadtools_protos.h"
 struct Gadget * CreateGadgetA ( ULONG kind , struct Gadget * gad , struct NewGadget * ng , const struct TagItem * taglist ) ;
 struct Gadget * CreateGadget ( ULONG kind , struct Gadget * gad , struct NewGadget * ng , ... ) ;
 void FreeGadgets ( struct Gadget * gad ) ;
 void GT_SetGadgetAttrsA ( struct Gadget * gad , struct Window * win , struct Requester * req , const struct TagItem * taglist ) ;
 void GT_SetGadgetAttrs ( struct Gadget * gad , struct Window * win , struct Requester * req , ... ) ;



 struct Menu * CreateMenusA ( const struct NewMenu * newmenu , struct TagItem * taglist ) ;
 struct Menu * CreateMenus ( const struct NewMenu * newmenu , ... ) ;
 void FreeMenus ( struct Menu * menu ) ;
 BOOL LayoutMenuItemsA ( struct MenuItem * firstitem , APTR vi , const struct TagItem * taglist ) ;
 BOOL LayoutMenuItems ( struct MenuItem * firstitem , APTR vi , ... ) ;
 BOOL LayoutMenusA ( struct Menu * firstmenu , APTR vi , const struct TagItem * taglist ) ;
 BOOL LayoutMenus ( struct Menu * firstmenu , APTR vi , ... ) ;



 struct IntuiMessage * GT_GetIMsg ( struct MsgPort * iport ) ;
 void GT_ReplyIMsg ( struct IntuiMessage * imsg ) ;
 void GT_RefreshWindow ( struct Window * win , struct Requester * req ) ;
 void GT_BeginRefresh ( struct Window * win ) ;
 void GT_EndRefresh ( struct Window * win , LONG complete ) ;
 struct IntuiMessage * GT_FilterIMsg ( const struct IntuiMessage * imsg ) ;
 struct IntuiMessage * GT_PostFilterIMsg ( struct IntuiMessage * imsg ) ;
 struct Gadget * CreateContext ( struct Gadget * * glistptr ) ;



 void DrawBevelBoxA ( struct RastPort * rport , LONG left , LONG top , LONG width , LONG height , const struct TagItem * taglist ) ;
 void DrawBevelBox ( struct RastPort * rport , LONG left , LONG top , LONG width , LONG height , ... ) ;



 APTR GetVisualInfoA ( struct Screen * screen , const struct TagItem * taglist ) ;
 APTR GetVisualInfo ( struct Screen * screen , ... ) ;
 void FreeVisualInfo ( APTR vi ) ;




 LONG SetDesignFontA ( APTR vi , struct TextAttr * tattr , const struct TagItem * tags ) ;
 LONG SetDesignFont ( APTR vi , struct TextAttr * tattr , ... ) ;
 LONG ScaleGadgetRectA ( struct NewGadget * ng , const struct TagItem * tags ) ;
 LONG ScaleGadgetRect ( struct NewGadget * ng , ... ) ;




 LONG GT_GetGadgetAttrsA ( struct Gadget * gad , struct Window * win , struct Requester * req , const struct TagItem * taglist ) ;
 LONG GT_GetGadgetAttrs ( struct Gadget * gad , struct Window * win , struct Requester * req , ... ) ;
#line 10 "proto\gadtools.h"
#pragma stdargs-off



 extern struct Library * GadToolsBase ;
#line 8 "inline\gadtools_protos.h"
 struct Gadget * __CreateGadgetA ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG kind , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct NewGadget * ng , __reg ( "a2" ) const struct TagItem * taglist ) = "\tjsr\t-30(a6)" ;



 struct Gadget * __CreateGadget ( __reg ( "a6" ) void * , __reg ( "d0" ) ULONG kind , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct NewGadget * ng , ... ) = "\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-30(a6)\n\tmovea.l\t(a7)+,a2" ;



 void __FreeGadgets ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gad ) = "\tjsr\t-36(a6)" ;


 void __GT_SetGadgetAttrsA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct Window * win , __reg ( "a2" ) struct Requester * req , __reg ( "a3" ) const struct TagItem * taglist ) = "\tjsr\t-42(a6)" ;



 void __GT_SetGadgetAttrs ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct Window * win , __reg ( "a2" ) struct Requester * req , ... ) = "\tmove.l\ta3,-(a7)\n\tlea\t4(a7),a3\n\tjsr\t-42(a6)\n\tmovea.l\t(a7)+,a3" ;



 struct Menu * __CreateMenusA ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewMenu * newmenu , __reg ( "a1" ) struct TagItem * taglist ) = "\tjsr\t-48(a6)" ;



 struct Menu * __CreateMenus ( __reg ( "a6" ) void * , __reg ( "a0" ) const struct NewMenu * newmenu , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-48(a6)\n\tmovea.l\t(a7)+,a1" ;



 void __FreeMenus ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Menu * menu ) = "\tjsr\t-54(a6)" ;


 BOOL __LayoutMenuItemsA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MenuItem * firstitem , __reg ( "a1" ) APTR vi , __reg ( "a2" ) const struct TagItem * taglist ) = "\tjsr\t-60(a6)" ;



 BOOL __LayoutMenuItems ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MenuItem * firstitem , __reg ( "a1" ) APTR vi , ... ) = "\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-60(a6)\n\tmovea.l\t(a7)+,a2" ;



 BOOL __LayoutMenusA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Menu * firstmenu , __reg ( "a1" ) APTR vi , __reg ( "a2" ) const struct TagItem * taglist ) = "\tjsr\t-66(a6)" ;



 BOOL __LayoutMenus ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Menu * firstmenu , __reg ( "a1" ) APTR vi , ... ) = "\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-66(a6)\n\tmovea.l\t(a7)+,a2" ;



 struct IntuiMessage * __GT_GetIMsg ( __reg ( "a6" ) void * , __reg ( "a0" ) struct MsgPort * iport ) = "\tjsr\t-72(a6)" ;


 void __GT_ReplyIMsg ( __reg ( "a6" ) void * , __reg ( "a1" ) struct IntuiMessage * imsg ) = "\tjsr\t-78(a6)" ;


 void __GT_RefreshWindow ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * win , __reg ( "a1" ) struct Requester * req ) = "\tjsr\t-84(a6)" ;


 void __GT_BeginRefresh ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * win ) = "\tjsr\t-90(a6)" ;


 void __GT_EndRefresh ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Window * win , __reg ( "d0" ) LONG complete ) = "\tjsr\t-96(a6)" ;


 struct IntuiMessage * __GT_FilterIMsg ( __reg ( "a6" ) void * , __reg ( "a1" ) const struct IntuiMessage * imsg ) = "\tjsr\t-102(a6)" ;


 struct IntuiMessage * __GT_PostFilterIMsg ( __reg ( "a6" ) void * , __reg ( "a1" ) struct IntuiMessage * imsg ) = "\tjsr\t-108(a6)" ;


 struct Gadget * __CreateContext ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * * glistptr ) = "\tjsr\t-114(a6)" ;


 void __DrawBevelBoxA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rport , __reg ( "d0" ) LONG left , __reg ( "d1" ) LONG top , __reg ( "d2" ) LONG width , __reg ( "d3" ) LONG height , __reg ( "a1" ) const struct TagItem * taglist ) = "\tjsr\t-120(a6)" ;



 void __DrawBevelBox ( __reg ( "a6" ) void * , __reg ( "a0" ) struct RastPort * rport , __reg ( "d0" ) LONG left , __reg ( "d1" ) LONG top , __reg ( "d2" ) LONG width , __reg ( "d3" ) LONG height , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-120(a6)\n\tmovea.l\t(a7)+,a1" ;



 APTR __GetVisualInfoA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , __reg ( "a1" ) const struct TagItem * taglist ) = "\tjsr\t-126(a6)" ;



 APTR __GetVisualInfo ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Screen * screen , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-126(a6)\n\tmovea.l\t(a7)+,a1" ;



 void __FreeVisualInfo ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR vi ) = "\tjsr\t-132(a6)" ;


 LONG __SetDesignFontA ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR vi , __reg ( "a1" ) struct TextAttr * tattr , __reg ( "a2" ) const struct TagItem * tags ) = "\tjsr\t-138(a6)" ;



 LONG __SetDesignFont ( __reg ( "a6" ) void * , __reg ( "a0" ) APTR vi , __reg ( "a1" ) struct TextAttr * tattr , ... ) = "\tmove.l\ta2,-(a7)\n\tlea\t4(a7),a2\n\tjsr\t-138(a6)\n\tmovea.l\t(a7)+,a2" ;



 LONG __ScaleGadgetRectA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct NewGadget * ng , __reg ( "a1" ) const struct TagItem * tags ) = "\tjsr\t-144(a6)" ;



 LONG __ScaleGadgetRect ( __reg ( "a6" ) void * , __reg ( "a0" ) struct NewGadget * ng , ... ) = "\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-144(a6)\n\tmovea.l\t(a7)+,a1" ;



 LONG __GT_GetGadgetAttrsA ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct Window * win , __reg ( "a2" ) struct Requester * req , __reg ( "a3" ) const struct TagItem * taglist ) = "\tjsr\t-174(a6)" ;



 LONG __GT_GetGadgetAttrs ( __reg ( "a6" ) void * , __reg ( "a0" ) struct Gadget * gad , __reg ( "a1" ) struct Window * win , __reg ( "a2" ) struct Requester * req , ... ) = "\tmove.l\ta3,-(a7)\n\tlea\t4(a7),a3\n\tjsr\t-174(a6)\n\tmovea.l\t(a7)+,a3" ;
#line 23 "clib\timer_protos.h"
 void AddTime ( TimeVal_Type * dest , const TimeVal_Type * src ) ;
 void SubTime ( TimeVal_Type * dest , const TimeVal_Type * src ) ;
 LONG CmpTime ( const TimeVal_Type * dest , const TimeVal_Type * src ) ;
 ULONG ReadEClock ( struct EClockVal * dest ) ;
 void GetSysTime ( TimeVal_Type * dest ) ;
#line 231 "src\helpers\listview_columns_api.c"
 extern struct Device * TimerBase ;













 BOOL iTidy_HandleListViewGadgetUp (
 struct Window * window ,
 struct Gadget * gadget ,
 WORD mouse_x ,
 WORD mouse_y ,
 struct List * entry_list ,
 struct List * display_list ,
 iTidy_ListViewState * state ,
 int font_height ,
 int font_width ,
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 iTidy_ListViewEvent * out_event ) ;

 BOOL iTidy_HandleListViewSort (
 struct Window * window ,
 struct Gadget * listview_gadget ,
 struct List * formatted_list ,
 struct List * entry_list ,
 iTidy_ListViewState * state ,
 int mouse_x ,
 int mouse_y ,
 int font_height ,
 int font_width ,
 iTidy_ColumnConfig * columns ,
 int num_columns ) ;





 void iTidy_InitListViewOptions ( iTidy_ListViewOptions * options )
 {
 if ( ! options ) {
 return ;
 }
 __asm_memset ( options , 0 , sizeof ( iTidy_ListViewOptions ) ) ;
 options -> mode = ITIDY_MODE_FULL ;
 }

 static BOOL itidy_is_valid_mode ( iTidy_ListViewMode mode )
 {
 switch ( mode ) {
 case ITIDY_MODE_FULL :
 case ITIDY_MODE_FULL_NO_SORT :
 case ITIDY_MODE_SIMPLE :
 case ITIDY_MODE_SIMPLE_PAGINATED :
 return 1 ;
 default :
 return 0 ;
 }
 }

 BOOL iTidy_ValidateListViewOptions ( const iTidy_ListViewOptions * options , const char * * out_error_text )
 {
 const char * error_text = ( ( void * ) 0 ) ;
 if ( ! options ) {
 error_text = "options pointer is NULL" ;
 } else if ( ! options -> columns ) {
 error_text = "columns array is NULL" ;
 } else if ( options -> num_columns <= 0 ) {
 error_text = "num_columns must be > 0" ;
 } else if ( options -> total_char_width <= 0 || options -> total_char_width > 500 ) {
 error_text = "total_char_width must be between 1 and 500 characters" ;
 } else if ( ! itidy_is_valid_mode ( options -> mode ) ) {
 error_text = "mode value is invalid" ;
 }

 if ( out_error_text ) {
 * out_error_text = error_text ;
 }

 return ( error_text == ( ( void * ) 0 ) ) ;
 }

 struct List * iTidy_FormatListViewColumnsEx ( const iTidy_ListViewOptions * options )
 {
 const char * error_text = ( ( void * ) 0 ) ;
 if ( ! iTidy_ValidateListViewOptions ( options , & error_text ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_FormatListViewColumnsEx: %s\n" , error_text ? error_text : "Invalid options" )
 ;
 return ( ( void * ) 0 ) ;
 }

 return iTidy_FormatListViewColumns (
 options -> columns ,
 options -> num_columns ,
 options -> entries ,
 options -> total_char_width ,
 options -> out_state ,
 options -> mode ,
 options -> page_size ,
 options -> current_page ,
 options -> out_total_pages ,
 options -> nav_direction
 ) ;
 }

 iTidy_ListViewSession * iTidy_ListViewSessionCreate ( const iTidy_ListViewOptions * options )
 {
 iTidy_ListViewSession * session ;
 const char * error_text = ( ( void * ) 0 ) ;
 if ( ! iTidy_ValidateListViewOptions ( options , & error_text ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_ListViewSessionCreate: %s\n" , error_text ? error_text : "Invalid options" )
 ;
 return ( ( void * ) 0 ) ;
 }

 session = ( iTidy_ListViewSession * ) whd_malloc_debug ( sizeof ( iTidy_ListViewSession ) , "src\\helpers\\listview_columns_api.c" , 353 ) ;
 if ( ! session ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_ListViewSessionCreate: out of memory\n" ) ;
 return ( ( void * ) 0 ) ;
 }

 __asm_memset ( session , 0 , sizeof ( iTidy_ListViewSession ) ) ;
 session -> options = * options ;
 session -> entry_list = options -> entries ;

 if ( options -> out_state != ( ( void * ) 0 ) ) {
 session -> state_target = options -> out_state ;
 session -> state = * ( options -> out_state ) ;
 } else {
 session -> state_target = & session -> state ;
 session -> options . out_state = & session -> state ;
 }

 return session ;
 }

 static void itidy_session_release_display ( iTidy_ListViewSession * session )
 {
 if ( session -> display_list ) {
 iTidy_FreeFormattedList ( session -> display_list ) ;
 session -> display_list = ( ( void * ) 0 ) ;
 }
 }

 static void itidy_session_release_state ( iTidy_ListViewSession * session )
 {
 if ( session -> state_target && * ( session -> state_target ) ) {
 iTidy_FreeListViewState ( * ( session -> state_target ) ) ;
 * ( session -> state_target ) = ( ( void * ) 0 ) ;
 }
 session -> state = ( ( void * ) 0 ) ;
 }

 struct List * iTidy_ListViewSessionFormat ( iTidy_ListViewSession * session )
 {
 if ( ! session ) {
 return ( ( void * ) 0 ) ;
 }


 itidy_session_release_display ( session ) ;
 if ( session -> formatted_once ) {
 itidy_session_release_state ( session ) ;
 }

 session -> options . entries = session -> entry_list ;
 session -> options . out_state = session -> state_target ;

 session -> display_list = iTidy_FormatListViewColumnsEx ( & session -> options ) ;
 if ( ! session -> display_list ) {
 return ( ( void * ) 0 ) ;
 }

 if ( session -> state_target ) {
 session -> state = * ( session -> state_target ) ;
 }

 session -> formatted_once = 1 ;
 return session -> display_list ;
 }

 void iTidy_ListViewSessionDestroy ( iTidy_ListViewSession * session )
 {
 if ( ! session ) {
 return ;
 }

 itidy_session_release_display ( session ) ;
 itidy_session_release_state ( session ) ;


 whd_free_debug ( session , "src\\helpers\\listview_columns_api.c" , 429 ) ;
 }





 typedef struct {
 BOOL sorting_disabled ;
 int default_sort_column ;
 } iTidy_SortPreparation ;

 typedef struct {
 BOOL pagination_enabled ;
 BOOL has_prev_page ;
 BOOL has_next_page ;
 int page_size ;
 int current_page ;
 int total_pages ;
 int total_entries ;
 int start_index ;
 int end_index ;
 } iTidy_PaginationInfo ;

 typedef struct {
 int current_page ;
 int total_pages ;
 int data_row_count ;
 } iTidy_DisplayPaginationInfo ;

 typedef struct {
 struct Window * window ;
 struct Gadget * gadget ;
 WORD mouse_x ;
 WORD mouse_y ;
 struct List * entry_list ;
 struct List * display_list ;
 iTidy_ListViewState * state ;
 int font_height ;
 int font_width ;
 iTidy_ColumnConfig * columns ;
 int num_columns ;
 iTidy_ListViewEvent * out_event ;
 LONG selected_row ;
 } iTidy_ListViewEventContext ;





 static void format_cell ( char * output , const char * text , int width ,
 iTidy_ColumnAlign align , BOOL is_path ) ;

 static struct List * iTidy_FormatListViewColumns_SimplePaginated (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 struct List * entries ,
 int total_char_width ,
 iTidy_ListViewState * * out_state ,
 int page_size ,
 int current_page ,
 int total_pages ,
 BOOL has_prev_page ,
 BOOL has_next_page ,
 int nav_direction ) ;

 static void format_header_row ( char * row_buffer , char * cell_buffer ,
 iTidy_ColumnConfig * columns , int num_columns ,
 int * col_widths , iTidy_ListViewState * state ) ;

 static void format_data_row ( char * row_buffer , char * cell_buffer ,
 iTidy_ListViewEntry * entry , iTidy_ColumnConfig * columns ,
 int num_columns , int * col_widths ) ;

 static int itidy_add_data_rows ( struct List * list ,
 struct List * entries ,
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 int * col_widths ,
 char * row_buffer ,
 char * cell_buffer ,
 BOOL pagination_enabled ,
 int start_index ,
 int end_index ) ;

 static void itidy_add_navigation_row ( struct List * list ,
 int total_char_width ,
 iTidy_RowType row_type ,
 int current_page ,
 int total_pages ) ;
 static void itidy_init_event_defaults ( iTidy_ListViewEvent * out_event ) ;
 static void itidy_extract_display_pagination_info ( struct List * display_list ,
 iTidy_ListViewState * state ,
 iTidy_DisplayPaginationInfo * info ) ;
 static int itidy_detect_navigation_direction ( struct Node * node ,
 iTidy_ListViewState * state ) ;
 static BOOL itidy_handle_header_click ( iTidy_ListViewEventContext * ctx ) ;
 static BOOL itidy_handle_navigation_click ( iTidy_ListViewEventContext * ctx ,
 struct Node * clicked_node ) ;
 static BOOL itidy_handle_data_row_click ( iTidy_ListViewEventContext * ctx ,
 struct Node * clicked_node ,
 BOOL has_previous_nav ,
 const iTidy_DisplayPaginationInfo * display_info ) ;
 static void itidy_record_row_click ( iTidy_ListViewEvent * out_event ,
 iTidy_ListViewEntry * entry ,
 int column ,
 iTidy_ListViewState * state ) ;

 static struct Node * create_display_node ( const char * text ) ;

















 static int compare_entries ( iTidy_ListViewEntry * a , iTidy_ListViewEntry * b , int col , iTidy_ColumnType type )
 {
 const char * key_a ;
 const char * key_b ;

 if ( ! a || ! b ) {
 return 0 ;
 }


 key_a = ( a -> sort_keys && col < a -> num_columns ) ? a -> sort_keys [ col ] : ( ( void * ) 0 ) ;
 key_b = ( b -> sort_keys && col < b -> num_columns ) ? b -> sort_keys [ col ] : ( ( void * ) 0 ) ;


 if ( key_a == ( ( void * ) 0 ) && key_b == ( ( void * ) 0 ) ) return 0 ;
 if ( key_a == ( ( void * ) 0 ) ) return - 1 ;
 if ( key_b == ( ( void * ) 0 ) ) return 1 ;


 if ( key_a [ 0 ] == '\0' && key_b [ 0 ] == '\0' ) return 0 ;
 if ( key_a [ 0 ] == '\0' ) return - 1 ;
 if ( key_b [ 0 ] == '\0' ) return 1 ;


 switch ( type ) {
 case ITIDY_COLTYPE_NUMBER :

 return ( int ) strtol ( ( key_a ) , ( char * * ) ( ( void * ) 0 ) , 10 ) - ( int ) strtol ( ( key_b ) , ( char * * ) ( ( void * ) 0 ) , 10 ) ;

 case ITIDY_COLTYPE_DATE :

 return __asm_strcmp ( key_a , key_b ) ;

 case ITIDY_COLTYPE_TEXT :
 default :

 return __asm_strcmp ( key_a , key_b ) ;
 }
 }














 static struct List * merge_lists ( struct List * left , struct List * right , int col , iTidy_ColumnType type , BOOL ascending )
 {
 struct List * result ;
 struct Node * node_left , * node_right ;
 iTidy_ListViewEntry * entry_left , * entry_right ;
 int cmp ;

 result = ( struct List * ) whd_malloc_debug ( sizeof ( struct List ) , "src\\helpers\\listview_columns_api.c" , 615 ) ;
 if ( ! result ) return ( ( void * ) 0 ) ;

 do { struct List * _list = ( result ) ; _list -> lh_Head = ( struct Node * ) & _list -> lh_Tail ; _list -> lh_Tail = ( ( void * ) 0 ) ; _list -> lh_TailPred = ( struct Node * ) & _list -> lh_Head ; } while ( 0 ) ;

 node_left = left -> lh_Head ;
 node_right = right -> lh_Head ;


 while ( node_left -> ln_Succ && node_right -> ln_Succ ) {
 entry_left = ( iTidy_ListViewEntry * ) node_left ;
 entry_right = ( iTidy_ListViewEntry * ) node_right ;

 cmp = compare_entries ( entry_left , entry_right , col , type ) ;


 if ( ! ascending ) {
 cmp = - cmp ;
 }

 if ( cmp <= 0 ) {

 struct Node * next = node_left -> ln_Succ ;
 __Remove ( SysBase , ( node_left ) ) ;
 __AddTail ( SysBase , ( result ) , ( node_left ) ) ;
 node_left = next ;
 } else {

 struct Node * next = node_right -> ln_Succ ;
 __Remove ( SysBase , ( node_right ) ) ;
 __AddTail ( SysBase , ( result ) , ( node_right ) ) ;
 node_right = next ;
 }
 }


 while ( node_left -> ln_Succ ) {
 struct Node * next = node_left -> ln_Succ ;
 __Remove ( SysBase , ( node_left ) ) ;
 __AddTail ( SysBase , ( result ) , ( node_left ) ) ;
 node_left = next ;
 }


 while ( node_right -> ln_Succ ) {
 struct Node * next = node_right -> ln_Succ ;
 __Remove ( SysBase , ( node_right ) ) ;
 __AddTail ( SysBase , ( result ) , ( node_right ) ) ;
 node_right = next ;
 }

 return result ;
 }












 void iTidy_SortListViewEntries ( struct List * list , int col , iTidy_ColumnType type , BOOL ascending )
 {
 struct List * left , * right , * sorted ;
 struct Node * node , * mid ;
 int count , i ;
 struct timeval startTime , endTime ;
 ULONG elapsedMicros , elapsedMillis ;
 static int sort_depth = 0 ;
 BOOL is_top_level = ( sort_depth == 0 ) ;


 if ( is_top_level && TimerBase ) {
 GetSysTime ( & startTime ) ;
 }

 sort_depth ++ ;

 if ( ! list ) {
 sort_depth -- ;
 return ;
 }


 count = 0 ;
 for ( node = list -> lh_Head ; node -> ln_Succ ; node = node -> ln_Succ ) {
 count ++ ;
 }


 if ( count <= 1 ) {
 return ;
 }


 left = ( struct List * ) whd_malloc_debug ( sizeof ( struct List ) , "src\\helpers\\listview_columns_api.c" , 714 ) ;
 right = ( struct List * ) whd_malloc_debug ( sizeof ( struct List ) , "src\\helpers\\listview_columns_api.c" , 715 ) ;

 if ( ! left || ! right ) {
 if ( left ) whd_free_debug ( left , "src\\helpers\\listview_columns_api.c" , 718 ) ;
 if ( right ) whd_free_debug ( right , "src\\helpers\\listview_columns_api.c" , 719 ) ;
 return ;
 }

 do { struct List * _list = ( left ) ; _list -> lh_Head = ( struct Node * ) & _list -> lh_Tail ; _list -> lh_Tail = ( ( void * ) 0 ) ; _list -> lh_TailPred = ( struct Node * ) & _list -> lh_Head ; } while ( 0 ) ;
 do { struct List * _list = ( right ) ; _list -> lh_Head = ( struct Node * ) & _list -> lh_Tail ; _list -> lh_Tail = ( ( void * ) 0 ) ; _list -> lh_TailPred = ( struct Node * ) & _list -> lh_Head ; } while ( 0 ) ;


 for ( i = 0 ; i < count / 2 ; i ++ ) {
 node = __RemHead ( SysBase , ( list ) ) ;
 if ( node ) {
 __AddTail ( SysBase , ( left ) , ( node ) ) ;
 }
 }


 while ( ( node = __RemHead ( SysBase , ( list ) ) ) != ( ( void * ) 0 ) ) {
 __AddTail ( SysBase , ( right ) , ( node ) ) ;
 }


 iTidy_SortListViewEntries ( left , col , type , ascending ) ;
 iTidy_SortListViewEntries ( right , col , type , ascending ) ;


 sorted = merge_lists ( left , right , col , type , ascending ) ;


 if ( sorted ) {
 while ( ( node = __RemHead ( SysBase , ( sorted ) ) ) != ( ( void * ) 0 ) ) {
 __AddTail ( SysBase , ( list ) , ( node ) ) ;
 }
 whd_free_debug ( sorted , "src\\helpers\\listview_columns_api.c" , 751 ) ;
 }


 whd_free_debug ( left , "src\\helpers\\listview_columns_api.c" , 755 ) ;
 whd_free_debug ( right , "src\\helpers\\listview_columns_api.c" , 756 ) ;

 sort_depth -- ;


 if ( is_top_level && TimerBase ) {
 GetSysTime ( & endTime ) ;


 elapsedMicros = ( ( endTime . tv_secs - startTime . tv_secs ) * 1000000 ) +
 ( endTime . tv_micro - startTime . tv_micro ) ;
 elapsedMillis = elapsedMicros / 1000 ;


 if ( is_performance_logging_enabled ( ) ) {
 const char * type_str = ( type == ITIDY_COLTYPE_NUMBER ) ? "NUMBER" :
 ( type == ITIDY_COLTYPE_DATE ) ? "DATE" : "TEXT" ;
 const char * dir_str = ascending ? "ASC" : "DESC" ;

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "[PERF] ListView sort (col %d, %s, %s): %lu.%03lu ms\n" , col , type_str , dir_str , elapsedMillis , elapsedMicros % 1000 )
 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "       Entries: %d\n" , count ) ;
 if ( count > 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "       Time per entry: %lu microseconds\n" , elapsedMicros / count )
 ;
 }

 printf ( "  [TIMING] Sort (%s/%s): %lu.%03lu ms (%d entries)\n" ,
 type_str , dir_str , elapsedMillis , elapsedMicros % 1000 , count ) ;
 }
 }
 }





 BOOL iTidy_CalculateColumnWidths (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 struct List * entries ,
 int total_char_width ,
 int * out_widths )
 {
 int i , col ;
 int separator_total ;
 int used_width ;
 int available_width ;
 int flexible_col = - 1 ;
 int flexible_count = 0 ;
 int max_len ;
 struct Node * node ;
 iTidy_ListViewEntry * entry ;
 struct timeval startTime , endTime ;
 ULONG elapsedMicros , elapsedMillis ;
 int entry_count = 0 ;


 if ( TimerBase ) {
 GetSysTime ( & startTime ) ;
 }

 if ( ! columns || ! out_widths || num_columns <= 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_CalculateColumnWidths: Invalid parameters (columns=%p, out_widths=%p, num_columns=%d)\n" , columns , out_widths , num_columns )
 ;
 return 0 ;
 }


 flexible_count = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {

 if ( columns [ col ] . title == ( ( void * ) 0 ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_CalculateColumnWidths: Column %d has NULL title - likely uninitialized!\n" , col ) ;
 return 0 ;
 }


 if ( columns [ col ] . min_width < 0 || columns [ col ] . min_width > 500 ||
 columns [ col ] . max_width < 0 || columns [ col ] . max_width > 500 ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_CalculateColumnWidths: Column %d (%s) has invalid width constraints (min=%d, max=%d)\n" , col , columns [ col ] . title , columns [ col ] . min_width , columns [ col ] . max_width )
 ;
 return 0 ;
 }


 if ( columns [ col ] . align < 0 || columns [ col ] . align > 2 ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_CalculateColumnWidths: Column %d (%s) has invalid alignment value %d\n" , col , columns [ col ] . title , ( int ) columns [ col ] . align )
 ;
 return 0 ;
 }


 if ( columns [ col ] . flexible ) {
 flexible_count ++ ;
 }
 }


 if ( flexible_count > 1 ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_CalculateColumnWidths: Invalid config - %d flexible columns detected!\n" , flexible_count ) ;
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "  This usually means the column array is uninitialized or corrupted.\n" ) ;
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "  Only ONE column should have flexible=TRUE.\n" ) ;


 for ( col = 0 ; col < num_columns ; col ++ ) {
 if ( columns [ col ] . flexible ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "    Column %d (%s): flexible=TRUE\n" , col , columns [ col ] . title ) ;
 }
 }
 return 0 ;
 }


 for ( col = 0 ; col < num_columns ; col ++ ) {


 max_len = columns [ col ] . title ? __asm_strlen ( columns [ col ] . title ) + 2 : 2 ;


 if ( entries ) {
 for ( node = entries -> lh_Head ; node -> ln_Succ ; node = node -> ln_Succ ) {
 entry = ( iTidy_ListViewEntry * ) node ;

 if ( entry -> display_data && col < entry -> num_columns && entry -> display_data [ col ] ) {
 int len = __asm_strlen ( entry -> display_data [ col ] ) ;
 if ( len > max_len ) {
 max_len = len ;
 }
 }
 }
 }


 if ( columns [ col ] . min_width > 0 && max_len < columns [ col ] . min_width ) {
 max_len = columns [ col ] . min_width ;
 }


 if ( columns [ col ] . max_width > 0 && max_len > columns [ col ] . max_width ) {
 max_len = columns [ col ] . max_width ;
 }

 out_widths [ col ] = max_len ;


 if ( columns [ col ] . flexible ) {
 if ( flexible_col >= 0 ) {

 log_message ( LOG_GUI , LOG_LEVEL_WARNING , "Multiple flexible columns detected (col %d and %d), using first\n" , flexible_col , col )
 ;
 } else {
 flexible_col = col ;
 }
 }
 }


 separator_total = ( num_columns - 1 ) * 3 ;


 used_width = separator_total ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 used_width += out_widths [ col ] ;
 }


 if ( total_char_width > 0 && flexible_col >= 0 ) {
 available_width = total_char_width - separator_total ;


 int fixed_width = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 if ( col != flexible_col ) {
 fixed_width += out_widths [ col ] ;
 }
 }


 int flex_width = available_width - fixed_width ;


 if ( columns [ flexible_col ] . min_width > 0 && flex_width < columns [ flexible_col ] . min_width ) {
 flex_width = columns [ flexible_col ] . min_width ;
 }
 if ( columns [ flexible_col ] . max_width > 0 && flex_width > columns [ flexible_col ] . max_width ) {
 flex_width = columns [ flexible_col ] . max_width ;
 }

 out_widths [ flexible_col ] = flex_width ;
 }


 if ( TimerBase ) {
 GetSysTime ( & endTime ) ;


 if ( entries ) {
 for ( node = entries -> lh_Head ; node -> ln_Succ ; node = node -> ln_Succ ) {
 entry_count ++ ;
 }
 }


 elapsedMicros = ( ( endTime . tv_secs - startTime . tv_secs ) * 1000000 ) +
 ( endTime . tv_micro - startTime . tv_micro ) ;
 elapsedMillis = elapsedMicros / 1000 ;


 if ( is_performance_logging_enabled ( ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "[PERF] Column width calculation: %lu.%03lu ms\n" , elapsedMillis , elapsedMicros % 1000 )
 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "       Entries: %d, Columns: %d\n" , entry_count , num_columns ) ;
 if ( entry_count > 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "       Time per entry: %lu microseconds\n" , elapsedMicros / entry_count )
 ;
 }

 printf ( "  [TIMING] Column width calc: %lu.%03lu ms (%d entries x %d cols)\n" ,
 elapsedMillis , elapsedMicros % 1000 , entry_count , num_columns ) ;
 }
 }

 return 1 ;
 }





























 static void format_cell ( char * output , const char * text , int width , iTidy_ColumnAlign align , BOOL is_path )
 {
 int len ;
 int padding ;
 int i ;
 char truncated [ 256 ] ;
 char path_abbreviated [ 256 ] ;
 const char * display_text ;

 if ( ! output ) return ;


 if ( ! text ) {
 text = "" ;
 }

 len = __asm_strlen ( text ) ;


 if ( is_path && len > width ) {

 if ( iTidy_ShortenPathWithParentDir ( text , path_abbreviated , width ) ) {

 display_text = path_abbreviated ;
 len = __asm_strlen ( path_abbreviated ) ;
 } else {

 display_text = text ;
 }
 } else {
 display_text = text ;
 }


 if ( len > width ) {
 if ( width >= 3 ) {

 __asm_strncpy ( truncated , display_text , width - 3 ) ;
 truncated [ width - 3 ] = '\0' ;
 __asm_strcat ( truncated , "..." ) ;
 } else {

 __asm_strncpy ( truncated , display_text , width ) ;
 truncated [ width ] = '\0' ;
 }
 display_text = truncated ;
 len = width ;
 }


 padding = width - len ;


 switch ( align ) {
 case ITIDY_ALIGN_RIGHT :

 for ( i = 0 ; i < padding ; i ++ ) {
 output [ i ] = ' ' ;
 }
 __asm_strcpy ( output + padding , display_text ) ;
 break ;

 case ITIDY_ALIGN_CENTER :

 {
 int left_pad = padding / 2 ;
 int right_pad = padding - left_pad ;
 for ( i = 0 ; i < left_pad ; i ++ ) {
 output [ i ] = ' ' ;
 }
 __asm_strcpy ( output + left_pad , display_text ) ;
 for ( i = 0 ; i < right_pad ; i ++ ) {
 output [ left_pad + len + i ] = ' ' ;
 }
 output [ width ] = '\0' ;
 }
 break ;

 case ITIDY_ALIGN_LEFT :
 default :

 __asm_strcpy ( output , display_text ) ;
 for ( i = len ; i < width ; i ++ ) {
 output [ i ] = ' ' ;
 }
 output [ width ] = '\0' ;
 break ;
 }
 }





 static struct Node * create_display_node ( const char * text )
 {
 struct Node * node ;
 char * text_copy ;

 if ( ! text ) return ( ( void * ) 0 ) ;

 node = ( struct Node * ) whd_malloc_debug ( sizeof ( struct Node ) , "src\\helpers\\listview_columns_api.c" , 1111 ) ;
 if ( ! node ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate display node\n" ) ;
 return ( ( void * ) 0 ) ;
 }

 __asm_memset ( node , 0 , sizeof ( struct Node ) ) ;

 text_copy = ( char * ) whd_malloc_debug ( __asm_strlen ( text ) + 1 , "src\\helpers\\listview_columns_api.c" , 1119 ) ;
 if ( ! text_copy ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate node text\n" ) ;
 whd_free_debug ( node , "src\\helpers\\listview_columns_api.c" , 1122 ) ;
 return ( ( void * ) 0 ) ;
 }

 __asm_strcpy ( text_copy , text ) ;
 node -> ln_Name = text_copy ;

 return node ;
 }





 static struct List * iTidy_FormatListViewColumns_SimplePaginated (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 struct List * entries ,
 int total_char_width ,
 iTidy_ListViewState * * out_state ,
 int page_size ,
 int current_page ,
 int total_pages ,
 BOOL has_prev_page ,
 BOOL has_next_page ,
 int nav_direction )
 {
 struct List * list ;
 struct Node * node , * entry_node ;
 iTidy_ListViewEntry * entry ;
 char * row_buffer ;
 char * cell_buffer ;
 int * col_widths ;
 int buffer_size ;
 int col , pos ;
 int row_count ;
 int start_index , end_index ;
 int current_index ;

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "iTidy_FormatListViewColumns_SimplePaginated: page %d of %d, page_size=%d\n" , current_page , total_pages , page_size )
 ;


 list = ( struct List * ) whd_malloc_debug ( sizeof ( struct List ) , "src\\helpers\\listview_columns_api.c" , 1165 ) ;
 if ( ! list ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate list\n" ) ;
 return ( ( void * ) 0 ) ;
 }
 do { struct List * _list = ( list ) ; _list -> lh_Head = ( struct Node * ) & _list -> lh_Tail ; _list -> lh_Tail = ( ( void * ) 0 ) ; _list -> lh_TailPred = ( struct Node * ) & _list -> lh_Head ; } while ( 0 ) ;


 col_widths = ( int * ) whd_malloc_debug ( sizeof ( int ) * num_columns , "src\\helpers\\listview_columns_api.c" , 1173 ) ;
 if ( ! col_widths ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate column widths\n" ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 1176 ) ;
 return ( ( void * ) 0 ) ;
 }


 if ( ! iTidy_CalculateColumnWidths ( columns , num_columns , entries ,
 total_char_width , col_widths ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to calculate column widths\n" ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 1184 ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 1185 ) ;
 return ( ( void * ) 0 ) ;
 }


 buffer_size = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 buffer_size += col_widths [ col ] ;
 }
 buffer_size += ( num_columns - 1 ) * 3 ;
 buffer_size += 10 ;


 row_buffer = ( char * ) whd_malloc_debug ( buffer_size , "src\\helpers\\listview_columns_api.c" , 1198 ) ;
 cell_buffer = ( char * ) whd_malloc_debug ( 256 , "src\\helpers\\listview_columns_api.c" , 1199 ) ;

 if ( ! row_buffer || ! cell_buffer ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate buffers\n" ) ;
 if ( row_buffer ) whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 1203 ) ;
 if ( cell_buffer ) whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 1204 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 1205 ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 1206 ) ;
 return ( ( void * ) 0 ) ;
 }


 format_header_row ( row_buffer , cell_buffer , columns , num_columns , col_widths , ( ( void * ) 0 ) ) ;
 node = create_display_node ( row_buffer ) ;
 if ( node ) __AddTail ( SysBase , ( list ) , ( node ) ) ;


 pos = __asm_strlen ( row_buffer ) ;
 __asm_memset ( row_buffer , '-' , pos + 4 ) ;
 row_buffer [ pos + 4 ] = '\0' ;
 node = create_display_node ( row_buffer ) ;
 if ( node ) __AddTail ( SysBase , ( list ) , ( node ) ) ;


 if ( has_prev_page ) {
 itidy_add_navigation_row ( list ,
 total_char_width ,
 ITIDY_ROW_NAV_PREV ,
 current_page ,
 total_pages ) ;
 }


 start_index = ( current_page - 1 ) * page_size ;
 end_index = start_index + page_size ;
 current_index = 0 ;
 row_count = 0 ;

 if ( entries ) {
 for ( entry_node = entries -> lh_Head ; entry_node -> ln_Succ ; entry_node = entry_node -> ln_Succ ) {

 if ( current_index < start_index ) {
 current_index ++ ;
 continue ;
 }


 if ( current_index >= end_index ) {
 break ;
 }


 entry = ( iTidy_ListViewEntry * ) entry_node ;
 format_data_row ( row_buffer , cell_buffer , entry , columns , num_columns , col_widths ) ;
 node = create_display_node ( row_buffer ) ;
 if ( node ) __AddTail ( SysBase , ( list ) , ( node ) ) ;

 current_index ++ ;
 row_count ++ ;
 }
 }


 if ( has_next_page ) {
 itidy_add_navigation_row ( list ,
 total_char_width ,
 ITIDY_ROW_NAV_NEXT ,
 current_page ,
 total_pages ) ;
 }


 whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 1271 ) ;
 whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 1272 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 1273 ) ;

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Simple paginated mode: formatted %d data rows\n" , row_count ) ;


 if ( out_state && ( has_prev_page || has_next_page ) ) {
 iTidy_ListViewState * state ;
 struct Node * node ;
 int display_row_count = 0 ;


 for ( node = list -> lh_Head ; node -> ln_Succ ; node = node -> ln_Succ ) {
 display_row_count ++ ;
 }


 state = ( iTidy_ListViewState * ) whd_malloc_debug ( sizeof ( iTidy_ListViewState ) , "src\\helpers\\listview_columns_api.c" , 1289 ) ;
 if ( state ) {
 __asm_memset ( state , 0 , sizeof ( iTidy_ListViewState ) ) ;
 state -> current_page = current_page ;
 state -> total_pages = total_pages ;
 state -> last_nav_direction = nav_direction ;
 state -> sorting_disabled = 1 ;
 state -> num_columns = 0 ;
 state -> columns = ( ( void * ) 0 ) ;


 if ( display_row_count > 2 ) {
 if ( nav_direction < 0 ) {

 state -> auto_select_row = 2 ;
 } else if ( nav_direction > 0 ) {

 state -> auto_select_row = display_row_count - 1 ;
 } else {

 state -> auto_select_row = 2 ;
 }

 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "[SIMPLE_PAGINATED] Auto-select row %d of %d (nav_dir=%d)\n" , state -> auto_select_row , display_row_count , nav_direction )
 ;
 } else {
 state -> auto_select_row = - 1 ;
 }

 * out_state = state ;
 } else {
 * out_state = ( ( void * ) 0 ) ;
 }
 } else if ( out_state ) {
 * out_state = ( ( void * ) 0 ) ;
 }

 return list ;
 }





 static void format_header_row ( char * row_buffer , char * cell_buffer , iTidy_ColumnConfig * columns ,
 int num_columns , int * col_widths , iTidy_ListViewState * state )
 {
 int col , pos ;
 char title_with_indicator [ 128 ] ;

 pos = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {

 if ( state && state -> columns [ col ] . sort_state != ITIDY_SORT_NONE ) {
 const char * indicator = ( state -> columns [ col ] . sort_state == ITIDY_SORT_ASCENDING ) ? " ^" : " v" ;
 snprintf ( title_with_indicator , sizeof ( title_with_indicator ) , "%s%s" ,
 columns [ col ] . title ? columns [ col ] . title : "" , indicator ) ;
 format_cell ( cell_buffer , title_with_indicator , col_widths [ col ] , columns [ col ] . align , 0 ) ;
 } else {
 format_cell ( cell_buffer , columns [ col ] . title , col_widths [ col ] , columns [ col ] . align , 0 ) ;
 }

 __asm_strcpy ( row_buffer + pos , cell_buffer ) ;
 pos += col_widths [ col ] ;

 if ( col < num_columns - 1 ) {
 __asm_strcpy ( row_buffer + pos , " | " ) ;
 pos += 3 ;
 }
 }
 row_buffer [ pos ] = '\0' ;
 }





 static void format_data_row ( char * row_buffer , char * cell_buffer ,
 iTidy_ListViewEntry * entry , iTidy_ColumnConfig * columns ,
 int num_columns , int * col_widths )
 {
 int col , pos ;

 pos = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 const char * cell_data = ( entry -> display_data && col < entry -> num_columns && entry -> display_data [ col ] ) ?
 entry -> display_data [ col ] : "" ;

 format_cell ( cell_buffer , cell_data , col_widths [ col ] , columns [ col ] . align , columns [ col ] . is_path ) ;
 __asm_strcpy ( row_buffer + pos , cell_buffer ) ;
 pos += col_widths [ col ] ;

 if ( col < num_columns - 1 ) {
 __asm_strcpy ( row_buffer + pos , " | " ) ;
 pos += 3 ;
 }
 }
 row_buffer [ pos ] = '\0' ;
 }

 static int itidy_add_data_rows ( struct List * list ,
 struct List * entries ,
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 int * col_widths ,
 char * row_buffer ,
 char * cell_buffer ,
 BOOL pagination_enabled ,
 int start_index ,
 int end_index )
 {
 struct Node * entry_node ;
 iTidy_ListViewEntry * entry ;
 int row_count = 0 ;
 int current_index = 0 ;

 if ( ! list || ! entries || ! columns || num_columns <= 0 || ! col_widths || ! row_buffer || ! cell_buffer ) {
 return 0 ;
 }

 for ( entry_node = entries -> lh_Head ; entry_node -> ln_Succ ; entry_node = entry_node -> ln_Succ ) {
 entry = ( iTidy_ListViewEntry * ) entry_node ;

 if ( pagination_enabled ) {
 if ( current_index < start_index || current_index >= end_index ) {
 current_index ++ ;
 continue ;
 }
 }
 current_index ++ ;

 format_data_row ( row_buffer , cell_buffer , entry , columns , num_columns , col_widths ) ;

 if ( entry -> node . ln_Name ) {
 whd_free_debug ( entry -> node . ln_Name , "src\\helpers\\listview_columns_api.c" , 1423 ) ;
 }
 entry -> node . ln_Name = ( char * ) whd_malloc_debug ( __asm_strlen ( row_buffer ) + 1 , "src\\helpers\\listview_columns_api.c" , 1425 ) ;
 if ( entry -> node . ln_Name ) {
 __asm_strcpy ( entry -> node . ln_Name , row_buffer ) ;
 }

 {
 iTidy_ListViewEntry * display_entry ;

 display_entry = ( iTidy_ListViewEntry * ) whd_malloc_debug ( sizeof ( iTidy_ListViewEntry ) , "src\\helpers\\listview_columns_api.c" , 1433 ) ;
 if ( display_entry ) {
 display_entry -> node . ln_Name = ( char * ) whd_malloc_debug ( __asm_strlen ( row_buffer ) + 1 , "src\\helpers\\listview_columns_api.c" , 1435 ) ;
 if ( display_entry -> node . ln_Name ) {
 __asm_strcpy ( display_entry -> node . ln_Name , row_buffer ) ;
 }

 display_entry -> display_data = entry -> display_data ;
 display_entry -> sort_keys = entry -> sort_keys ;
 display_entry -> num_columns = entry -> num_columns ;
 display_entry -> row_type = ITIDY_ROW_DATA ;

 __AddTail ( SysBase , ( list ) , ( ( struct Node * ) display_entry ) ) ;
 }
 }

 row_count ++ ;
 }

 return row_count ;
 }

 static void itidy_add_navigation_row ( struct List * list ,
 int total_char_width ,
 iTidy_RowType row_type ,
 int current_page ,
 int total_pages )
 {
 iTidy_ListViewEntry * nav_entry ;
 char * full_text ;
 char nav_row_text [ 256 ] ;
 const char * template_text ;

 if ( ! list || total_char_width <= 0 ) {
 return ;
 }

 template_text = ( row_type == ITIDY_ROW_NAV_PREV ) ?
 "<- Previous (Page %d of %d)" : "Next -> (Page %d of %d)" ;

 snprintf ( nav_row_text , sizeof ( nav_row_text ) , template_text ,
 current_page , total_pages ) ;

 full_text = ( char * ) whd_malloc_debug ( total_char_width + 1 , "src\\helpers\\listview_columns_api.c" , 1476 ) ;
 if ( ! full_text ) {
 return ;
 }

 format_cell ( full_text , nav_row_text , total_char_width , ITIDY_ALIGN_LEFT , 0 ) ;

 nav_entry = ( iTidy_ListViewEntry * ) whd_malloc_debug ( sizeof ( iTidy_ListViewEntry ) , "src\\helpers\\listview_columns_api.c" , 1483 ) ;
 if ( ! nav_entry ) {
 whd_free_debug ( full_text , "src\\helpers\\listview_columns_api.c" , 1485 ) ;
 return ;
 }

 nav_entry -> node . ln_Name = full_text ;
 nav_entry -> display_data = ( ( void * ) 0 ) ;
 nav_entry -> sort_keys = ( ( void * ) 0 ) ;
 nav_entry -> num_columns = 0 ;
 nav_entry -> row_type = row_type ;

 __AddTail ( SysBase , ( list ) , ( ( struct Node * ) & nav_entry -> node ) ) ;
 }

 static void itidy_init_event_defaults ( iTidy_ListViewEvent * out_event )
 {
 if ( ! out_event ) {
 return ;
 }

 out_event -> type = ITIDY_LV_EVENT_NONE ;
 out_event -> did_sort = 0 ;
 out_event -> sorted_column = - 1 ;
 out_event -> sort_order = ITIDY_SORT_NONE ;
 out_event -> entry = ( ( void * ) 0 ) ;
 out_event -> column = - 1 ;
 out_event -> display_value = ( ( void * ) 0 ) ;
 out_event -> sort_key = ( ( void * ) 0 ) ;
 out_event -> column_type = ITIDY_COLTYPE_TEXT ;
 out_event -> nav_direction = 0 ;
 }

 static int itidy_detect_navigation_direction ( struct Node * node ,
 iTidy_ListViewState * state )
 {
 iTidy_ListViewEntry * entry ;

 if ( ! node ) {
 return 0 ;
 }

 if ( state != ( ( void * ) 0 ) ) {
 entry = ( iTidy_ListViewEntry * ) node ;
 if ( entry -> row_type == ITIDY_ROW_NAV_PREV ) {
 return - 1 ;
 }
 if ( entry -> row_type == ITIDY_ROW_NAV_NEXT ) {
 return 1 ;
 }
 }

 if ( node -> ln_Name ) {
 if ( strstr ( node -> ln_Name , "Previous" ) ) {
 return - 1 ;
 }
 if ( strstr ( node -> ln_Name , "Next" ) ) {
 return 1 ;
 }
 }

 return 0 ;
 }

 static void itidy_extract_display_pagination_info ( struct List * display_list ,
 iTidy_ListViewState * state ,
 iTidy_DisplayPaginationInfo * info )
 {
 struct Node * node ;
 LONG row_index = 0 ;

 if ( ! info ) {
 return ;
 }

 info -> current_page = ( state && state -> current_page > 0 ) ? state -> current_page : 1 ;
 info -> total_pages = ( state && state -> total_pages > 0 ) ? state -> total_pages : 1 ;
 info -> data_row_count = 0 ;

 if ( ! display_list ) {
 return ;
 }

 for ( node = display_list -> lh_Head ; node -> ln_Succ ; node = node -> ln_Succ ) {
 if ( row_index >= 2 ) {
 int direction = itidy_detect_navigation_direction ( node , state ) ;
 if ( direction != 0 ) {
 if ( state == ( ( void * ) 0 ) && node -> ln_Name ) {
 const char * page_str = strstr ( node -> ln_Name , "Page" ) ;
 if ( page_str ) {
 int page = info -> current_page ;
 int total = info -> total_pages ;
 if ( sscanf ( page_str , "Page %d of %d" , & page , & total ) == 2 ) {
 if ( page > 0 ) info -> current_page = page ;
 if ( total > 0 ) info -> total_pages = total ;
 }
 }
 }
 row_index ++ ;
 continue ;
 }

 info -> data_row_count ++ ;
 }
 row_index ++ ;
 }
 }

 static BOOL itidy_handle_header_click ( iTidy_ListViewEventContext * ctx )
 {
 int header_top ;
 int header_height ;
 int clicked_col ;
 BOOL sorted ;

 if ( ! ctx || ! ctx -> state || ! ctx -> out_event ) {
 return 0 ;
 }

 if ( ctx -> state -> current_page > 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Header click ignored - sorting disabled during pagination\n" ) ;
 return 0 ;
 }

 if ( ctx -> state -> sorting_disabled ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Header click ignored - sorting disabled by user preference\n" ) ;
 return 0 ;
 }

 header_top = ctx -> gadget -> TopEdge ;
 header_height = ctx -> font_height ;
 clicked_col = iTidy_GetClickedColumn ( ctx -> state , ctx -> mouse_x , ctx -> gadget -> LeftEdge ) ;

 if ( clicked_col < 0 || clicked_col >= ctx -> num_columns ) {
 return 0 ;
 }

 sorted = iTidy_ResortListViewByClick (
 ctx -> display_list ,
 ctx -> entry_list ,
 ctx -> state ,
 ctx -> mouse_x ,
 ctx -> mouse_y ,
 header_top ,
 header_height ,
 ctx -> gadget -> LeftEdge ,
 ctx -> font_width ,
 ctx -> columns
 ) ;

 if ( ! sorted ) {
 return 0 ;
 }

 ctx -> out_event -> type = ITIDY_LV_EVENT_HEADER_SORTED ;
 ctx -> out_event -> did_sort = 1 ;
 ctx -> out_event -> sorted_column = clicked_col ;
 ctx -> out_event -> sort_order = ctx -> state -> columns [ clicked_col ] . sort_state ;
 return 1 ;
 }

 static BOOL itidy_handle_navigation_click ( iTidy_ListViewEventContext * ctx ,
 struct Node * clicked_node )
 {
 int direction ;

 if ( ! ctx || ! ctx -> out_event ) {
 return 0 ;
 }

 direction = itidy_detect_navigation_direction ( clicked_node , ctx -> state ) ;
 if ( direction == 0 ) {
 return 0 ;
 }

 if ( ctx -> state ) {
 if ( direction < 0 ) {
 if ( ctx -> state -> current_page <= 1 ) {
 return 0 ;
 }
 ctx -> state -> current_page -- ;
 } else {
 if ( ctx -> state -> current_page >= ctx -> state -> total_pages ) {
 return 0 ;
 }
 ctx -> state -> current_page ++ ;
 }
 ctx -> state -> last_nav_direction = direction ;
 }

 ctx -> out_event -> type = ITIDY_LV_EVENT_NAV_HANDLED ;
 ctx -> out_event -> nav_direction = direction ;
 ctx -> out_event -> entry = ( ( void * ) 0 ) ;
 ctx -> out_event -> column = - 1 ;
 ctx -> out_event -> display_value = ( ( void * ) 0 ) ;
 ctx -> out_event -> sort_key = ( ( void * ) 0 ) ;
 ctx -> out_event -> column_type = ITIDY_COLTYPE_TEXT ;
 return 1 ;
 }

 static BOOL itidy_handle_data_row_click ( iTidy_ListViewEventContext * ctx ,
 struct Node * clicked_node ,
 BOOL has_previous_nav ,
 const iTidy_DisplayPaginationInfo * display_info )
 {
 iTidy_ListViewEntry * actual_entry = ( ( void * ) 0 ) ;
 struct Node * entry_node ;
 int row_offset ;
 int clicked_display_row ;
 int entry_index ;
 int page_size ;
 int current_page ;
 int entry_idx = 0 ;

 if ( ! ctx || ! ctx -> entry_list || ! ctx -> out_event ) {
 return 0 ;
 }

 row_offset = has_previous_nav ? 3 : 2 ;
 clicked_display_row = ctx -> selected_row - row_offset ;
 if ( clicked_display_row < 0 ) {
 return 0 ;
 }

 page_size = ( display_info && display_info -> data_row_count > 0 ) ?
 display_info -> data_row_count : 0 ;
 current_page = ( display_info && display_info -> current_page > 0 ) ?
 display_info -> current_page : 1 ;

 entry_index = clicked_display_row ;
 if ( current_page > 1 && page_size > 0 ) {
 entry_index = ( ( current_page - 1 ) * page_size ) + clicked_display_row ;
 }

 for ( entry_node = ctx -> entry_list -> lh_Head ; entry_node -> ln_Succ ; entry_node = entry_node -> ln_Succ ) {
 if ( entry_idx == entry_index ) {
 actual_entry = ( iTidy_ListViewEntry * ) entry_node ;
 break ;
 }
 entry_idx ++ ;
 }

 if ( ! actual_entry ) {
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "ERROR: Could not find entry at entry_index=%d\n" , entry_index ) ;
 return 0 ;
 }

 itidy_record_row_click ( ctx -> out_event ,
 actual_entry ,
 iTidy_GetClickedColumn ( ctx -> state , ctx -> mouse_x , ctx -> gadget -> LeftEdge ) ,
 ctx -> state ) ;

 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "Data row clicked: row %ld, entry_index=%d, current_page=%d\n" , ctx -> selected_row , entry_index , current_page )
 ;
 return 1 ;
 }

 static void itidy_record_row_click ( iTidy_ListViewEvent * out_event ,
 iTidy_ListViewEntry * entry ,
 int column ,
 iTidy_ListViewState * state )
 {
 out_event -> type = ITIDY_LV_EVENT_ROW_CLICK ;
 out_event -> entry = entry ;
 out_event -> column = column ;

 if ( entry && column >= 0 && column < entry -> num_columns ) {
 out_event -> display_value = entry -> display_data ? entry -> display_data [ column ] : ( ( void * ) 0 ) ;
 out_event -> sort_key = entry -> sort_keys ? entry -> sort_keys [ column ] : ( ( void * ) 0 ) ;
 if ( state && column < state -> num_columns ) {
 out_event -> column_type = state -> columns [ column ] . column_type ;
 } else {
 out_event -> column_type = ITIDY_COLTYPE_TEXT ;
 }
 } else {
 out_event -> display_value = ( ( void * ) 0 ) ;
 out_event -> sort_key = ( ( void * ) 0 ) ;
 out_event -> column_type = ITIDY_COLTYPE_TEXT ;
 }
 }

 static BOOL itidy_add_header_and_separator ( struct List * list ,
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 int * col_widths ,
 iTidy_ListViewState * state ,
 char * row_buffer ,
 char * cell_buffer )
 {
 struct Node * node ;
 int pos ;

 if ( ! list || ! columns || num_columns <= 0 || ! col_widths || ! row_buffer || ! cell_buffer ) {
 return 0 ;
 }

 format_header_row ( row_buffer , cell_buffer , columns , num_columns , col_widths , state ) ;

 node = create_display_node ( row_buffer ) ;
 if ( ! node ) {
 return 0 ;
 }
 __AddTail ( SysBase , ( list ) , ( node ) ) ;

 pos = __asm_strlen ( row_buffer ) ;
 __asm_memset ( row_buffer , '-' , pos + 4 ) ;
 row_buffer [ pos + 4 ] = '\0' ;

 node = create_display_node ( row_buffer ) ;
 if ( ! node ) {
 return 0 ;
 }
 __AddTail ( SysBase , ( list ) , ( node ) ) ;

 return 1 ;
 }

 static void itidy_prepare_sort ( iTidy_ColumnConfig * columns ,
 int num_columns ,
 struct List * entries ,
 iTidy_ListViewMode mode ,
 iTidy_SortPreparation * result )
 {
 BOOL sorting_disabled = ( mode == ITIDY_MODE_FULL_NO_SORT ||
 mode == ITIDY_MODE_SIMPLE ||
 mode == ITIDY_MODE_SIMPLE_PAGINATED ) ;
 int default_sort_col = - 1 ;

 if ( sorting_disabled ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Sorting disabled for mode=%d\n" , mode ) ;
 }

 for ( int col = 0 ; col < num_columns ; col ++ ) {
 if ( columns [ col ] . default_sort != ITIDY_SORT_NONE ) {
 default_sort_col = col ;
 break ;
 }
 }

 if ( ! sorting_disabled && default_sort_col >= 0 && entries ) {
 BOOL ascending = ( columns [ default_sort_col ] . default_sort == ITIDY_SORT_ASCENDING ) ;
 iTidy_SortListViewEntries ( entries ,
 default_sort_col ,
 columns [ default_sort_col ] . sort_type ,
 ascending ) ;
 }

 if ( result ) {
 result -> sorting_disabled = sorting_disabled ;
 result -> default_sort_column = default_sort_col ;
 }
 }

 static void itidy_compute_pagination ( struct List * entries ,
 iTidy_ListViewMode mode ,
 int requested_page_size ,
 int requested_current_page ,
 int * out_total_pages ,
 iTidy_PaginationInfo * result )
 {
 iTidy_PaginationInfo info ;
 struct Node * entry_node ;
 BOOL pagination_requested ;
 BOOL has_entries = ( entries != ( ( void * ) 0 ) ) ;

 __asm_memset ( & info , 0 , sizeof ( info ) ) ;
 info . page_size = requested_page_size ;
 info . current_page = ( requested_current_page > 0 ) ? requested_current_page : 1 ;
 info . total_pages = 1 ;
 info . start_index = 0 ;
 info . end_index = 0 ;

 if ( has_entries ) {
 for ( entry_node = entries -> lh_Head ; entry_node -> ln_Succ ; entry_node = entry_node -> ln_Succ ) {
 info . total_entries ++ ;
 }
 info . end_index = info . total_entries ;
 }

 pagination_requested = ( mode == ITIDY_MODE_SIMPLE_PAGINATED ) ||
 ( mode == ITIDY_MODE_FULL && requested_page_size > 0 ) ;

 if ( pagination_requested && has_entries ) {
 if ( info . page_size <= 0 ) {
 info . page_size = 50 ;
 }

 if ( info . total_entries > info . page_size ) {
 info . pagination_enabled = 1 ;
 info . total_pages = ( info . total_entries + info . page_size - 1 ) / info . page_size ;
 if ( info . total_pages < 1 ) {
 info . total_pages = 1 ;
 }

 if ( info . current_page < 1 ) info . current_page = 1 ;
 if ( info . current_page > info . total_pages ) info . current_page = info . total_pages ;

 info . start_index = ( info . current_page - 1 ) * info . page_size ;
 info . end_index = info . start_index + info . page_size ;
 if ( info . end_index > info . total_entries ) {
 info . end_index = info . total_entries ;
 }

 info . has_prev_page = ( info . current_page > 1 ) ;
 info . has_next_page = ( info . current_page < info . total_pages ) ;

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Pagination ACTIVE (mode=%d): %d entries, showing page %d of %d (entries %d-%d)\n" , mode , info . total_entries , info . current_page , info . total_pages , info . start_index , ( info . end_index > 0 ) ? ( info . end_index - 1 ) : 0 )

 ;
 } else {
 info . pagination_enabled = 0 ;
 info . has_prev_page = 0 ;
 info . has_next_page = 0 ;
 info . start_index = 0 ;
 info . end_index = info . total_entries ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Pagination DISABLED: %d entries <= page_size %d\n" , info . total_entries , info . page_size )
 ;
 }
 } else {
 info . pagination_enabled = 0 ;
 info . has_prev_page = 0 ;
 info . has_next_page = 0 ;
 if ( ! has_entries ) {
 info . start_index = 0 ;
 info . end_index = 0 ;
 }
 }

 if ( out_total_pages ) {
 * out_total_pages = info . total_pages ;
 }

 if ( result ) {
 * result = info ;
 }
 }





 struct List * iTidy_FormatListViewColumns (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 struct List * entries ,
 int total_char_width ,
 iTidy_ListViewState * * out_state ,
 iTidy_ListViewMode mode ,
 int page_size ,
 int current_page ,
 int * out_total_pages ,
 int nav_direction )
 {
 struct List * list ;
 struct Node * node , * entry_node ;
 iTidy_ListViewEntry * entry ;
 iTidy_ListViewState * state = ( ( void * ) 0 ) ;
 int * col_widths ;
 char * row_buffer ;
 char * cell_buffer ;
 int col , row_count ;
 int buffer_size ;
 int pos , char_pos ;
 int default_sort_col = - 1 ;
 struct timeval startTime , endTime , cp1Time , cp2Time , cp3Time ;
 ULONG elapsedMicros , elapsedMillis ;
 ULONG sortMicros = 0 , widthMicros = 0 , formatMicros = 0 ;
 int entry_count = 0 ;
 int total_pages = 1 ;
 int start_index = 0 ;
 int end_index = 0 ;
 BOOL has_prev_page = 0 ;
 BOOL has_next_page = 0 ;


 if ( TimerBase ) {
 GetSysTime ( & startTime ) ;
 }

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "iTidy_FormatListViewColumns: num_columns=%d, width=%d, mode=%d, page_size=%d, current_page=%d\n" , num_columns , total_char_width , mode , page_size , current_page )
 ;


 if ( ! columns || num_columns <= 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_FormatListViewColumns: Invalid parameters (columns=%p, num_columns=%d)\n" , columns , num_columns )
 ;
 return ( ( void * ) 0 ) ;
 }

 if ( total_char_width <= 0 || total_char_width > 500 ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_FormatListViewColumns: Invalid total_char_width=%d (expected 10-500)\n" , total_char_width )
 ;
 return ( ( void * ) 0 ) ;
 }


 BOOL simple_mode = ( mode == ITIDY_MODE_SIMPLE || mode == ITIDY_MODE_SIMPLE_PAGINATED ) ;
 iTidy_SortPreparation sort_info ;
 iTidy_PaginationInfo pagination_info ;
 itidy_prepare_sort ( columns , num_columns , entries , mode , & sort_info ) ;
 BOOL sorting_disabled = sort_info . sorting_disabled ;
 default_sort_col = sort_info . default_sort_column ;
 itidy_compute_pagination ( entries , mode , page_size , current_page , out_total_pages , & pagination_info ) ;
 BOOL pagination_enabled = pagination_info . pagination_enabled ;
 page_size = pagination_info . page_size ;
 current_page = pagination_info . current_page ;
 total_pages = pagination_info . total_pages ;
 start_index = pagination_info . start_index ;
 end_index = pagination_info . end_index ;
 has_prev_page = pagination_info . has_prev_page ;
 has_next_page = pagination_info . has_next_page ;


 if ( TimerBase ) {
 GetSysTime ( & cp1Time ) ;
 sortMicros = ( ( cp1Time . tv_secs - startTime . tv_secs ) * 1000000 ) +
 ( cp1Time . tv_micro - startTime . tv_micro ) ;
 }




 list = ( struct List * ) whd_malloc_debug ( sizeof ( struct List ) , "src\\helpers\\listview_columns_api.c" , 2005 ) ;
 if ( ! list ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate list\n" ) ;
 return ( ( void * ) 0 ) ;
 }
 do { struct List * _list = ( list ) ; _list -> lh_Head = ( struct Node * ) & _list -> lh_Tail ; _list -> lh_Tail = ( ( void * ) 0 ) ; _list -> lh_TailPred = ( struct Node * ) & _list -> lh_Head ; } while ( 0 ) ;


 if ( simple_mode ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Using simple mode formatter (pagination=%s)\n" , pagination_enabled ? "YES" : "NO" )
 ;



 if ( pagination_enabled ) {

 struct List * result ;
 result = iTidy_FormatListViewColumns_SimplePaginated (
 columns , num_columns , entries , total_char_width ,
 out_state ,
 page_size , current_page , total_pages ,
 has_prev_page , has_next_page , nav_direction
 ) ;


 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2030 ) ;

 return result ;
 }

 }


 col_widths = ( int * ) whd_malloc_debug ( sizeof ( int ) * num_columns , "src\\helpers\\listview_columns_api.c" , 2038 ) ;
 if ( ! col_widths ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate column widths\n" ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2041 ) ;
 return ( ( void * ) 0 ) ;
 }


 if ( ! iTidy_CalculateColumnWidths ( columns , num_columns , entries ,
 total_char_width , col_widths ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to calculate column widths\n" ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 2049 ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2050 ) ;
 return ( ( void * ) 0 ) ;
 }


 if ( TimerBase ) {
 GetSysTime ( & cp2Time ) ;
 widthMicros = ( ( cp2Time . tv_secs - cp1Time . tv_secs ) * 1000000 ) +
 ( cp2Time . tv_micro - cp1Time . tv_micro ) ;
 }


 if ( out_state ) {
 state = ( iTidy_ListViewState * ) whd_malloc_debug ( sizeof ( iTidy_ListViewState ) , "src\\helpers\\listview_columns_api.c" , 2063 ) ;
 if ( state ) {
 state -> num_columns = num_columns ;
 state -> separator_width = 3 ;
 state -> columns = ( iTidy_ColumnState * ) whd_malloc_debug ( sizeof ( iTidy_ColumnState ) * num_columns , "src\\helpers\\listview_columns_api.c" , 2067 ) ;
 state -> sorting_disabled = sorting_disabled ;


 if ( pagination_enabled ) {
 state -> current_page = current_page ;
 state -> total_pages = total_pages ;
 state -> last_nav_direction = nav_direction ;
 state -> auto_select_row = - 1 ;
 } else {
 state -> current_page = 0 ;
 state -> total_pages = 0 ;
 state -> last_nav_direction = 0 ;
 state -> auto_select_row = - 1 ;
 }

 if ( state -> columns ) {

 char_pos = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 state -> columns [ col ] . column_index = col ;
 state -> columns [ col ] . char_start = char_pos ;
 state -> columns [ col ] . char_width = col_widths [ col ] ;
 char_pos += col_widths [ col ] ;
 state -> columns [ col ] . char_end = char_pos ;


 state -> columns [ col ] . pixel_start = 0 ;
 state -> columns [ col ] . pixel_end = 0 ;


 if ( col == default_sort_col ) {
 state -> columns [ col ] . sort_state = columns [ col ] . default_sort ;
 } else {
 state -> columns [ col ] . sort_state = ITIDY_SORT_NONE ;
 }


 state -> columns [ col ] . column_type = columns [ col ] . sort_type ;

 char_pos += 3 ;
 }
 * out_state = state ;
 } else {
 whd_free_debug ( state , "src\\helpers\\listview_columns_api.c" , 2111 ) ;
 state = ( ( void * ) 0 ) ;
 }
 }
 }


 buffer_size = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 buffer_size += col_widths [ col ] ;
 }
 buffer_size += ( num_columns - 1 ) * 3 ;
 buffer_size += 10 ;


 row_buffer = ( char * ) whd_malloc_debug ( buffer_size , "src\\helpers\\listview_columns_api.c" , 2126 ) ;
 cell_buffer = ( char * ) whd_malloc_debug ( 256 , "src\\helpers\\listview_columns_api.c" , 2127 ) ;

 if ( ! row_buffer || ! cell_buffer ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate formatting buffers\n" ) ;
 if ( row_buffer ) whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 2131 ) ;
 if ( cell_buffer ) whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 2132 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 2133 ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2134 ) ;
 if ( state ) {
 if ( state -> columns ) whd_free_debug ( state -> columns , "src\\helpers\\listview_columns_api.c" , 2136 ) ;
 whd_free_debug ( state , "src\\helpers\\listview_columns_api.c" , 2137 ) ;
 }
 return ( ( void * ) 0 ) ;
 }


 if ( ! itidy_add_header_and_separator ( list , columns , num_columns , col_widths ,
 state , row_buffer , cell_buffer ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to build header/separator rows\n" ) ;
 goto cleanup_buffers ;
 }


 if ( page_size > 0 && has_prev_page ) {
 itidy_add_navigation_row ( list ,
 total_char_width ,
 ITIDY_ROW_NAV_PREV ,
 current_page ,
 total_pages ) ;
 }


 row_count = itidy_add_data_rows ( list ,
 entries ,
 columns ,
 num_columns ,
 col_widths ,
 row_buffer ,
 cell_buffer ,
 pagination_enabled ,
 start_index ,
 end_index ) ;


 if ( page_size > 0 && has_next_page ) {
 itidy_add_navigation_row ( list ,
 total_char_width ,
 ITIDY_ROW_NAV_NEXT ,
 current_page ,
 total_pages ) ;
 }


 whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 2180 ) ;
 whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 2181 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 2182 ) ;


 if ( TimerBase ) {
 GetSysTime ( & cp3Time ) ;
 formatMicros = ( ( cp3Time . tv_secs - cp2Time . tv_secs ) * 1000000 ) +
 ( cp3Time . tv_micro - cp2Time . tv_micro ) ;
 }


 if ( entries ) {
 for ( entry_node = entries -> lh_Head ; entry_node -> ln_Succ ; entry_node = entry_node -> ln_Succ ) {
 entry_count ++ ;
 }
 }


 if ( TimerBase ) {
 GetSysTime ( & endTime ) ;


 elapsedMicros = ( ( endTime . tv_secs - startTime . tv_secs ) * 1000000 ) +
 ( endTime . tv_micro - startTime . tv_micro ) ;
 elapsedMillis = elapsedMicros / 1000 ;


 if ( is_performance_logging_enabled ( ) ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "==== LISTVIEW FORMAT PERFORMANCE ====\n" ) ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "iTidy_FormatListViewColumns() total time: %lu.%03lu ms\n" , elapsedMillis , elapsedMicros % 1000 )
 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "  Entries: %d, Columns: %d, Rows created: %d\n" , entry_count , num_columns , row_count )
 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "  Breakdown:\n" ) ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "    Initial sort:    %lu.%03lu ms (%lu%%)\n" , sortMicros / 1000 , sortMicros % 1000 , elapsedMicros > 0 ? ( sortMicros * 100 / elapsedMicros ) : 0 )

 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "    Width calc:      %lu.%03lu ms (%lu%%)\n" , widthMicros / 1000 , widthMicros % 1000 , elapsedMicros > 0 ? ( widthMicros * 100 / elapsedMicros ) : 0 )

 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "    Entry format:    %lu.%03lu ms (%lu%%)\n" , formatMicros / 1000 , formatMicros % 1000 , elapsedMicros > 0 ? ( formatMicros * 100 / elapsedMicros ) : 0 )

 ;
 if ( entry_count > 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "  Time per entry: %lu microseconds\n" , elapsedMicros / entry_count )
 ;
 }
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "====================================\n" ) ;

 printf ( "  [TIMING] ListView format: %lu.%03lu ms total (%d entries)\n" ,
 elapsedMillis , elapsedMicros % 1000 , entry_count ) ;
 printf ( "           Sort: %lu.%03lu ms | Width: %lu.%03lu ms | Format: %lu.%03lu ms\n" ,
 sortMicros / 1000 , sortMicros % 1000 ,
 widthMicros / 1000 , widthMicros % 1000 ,
 formatMicros / 1000 , formatMicros % 1000 ) ;
 }
 }


 if ( state != ( ( void * ) 0 ) && ( has_prev_page || has_next_page ) ) {
 struct Node * node ;
 int display_row_count = 0 ;


 for ( node = list -> lh_Head ; node -> ln_Succ ; node = node -> ln_Succ ) {
 display_row_count ++ ;
 }


 if ( display_row_count > 2 ) {
 if ( state -> last_nav_direction < 0 ) {

 state -> auto_select_row = 2 ;
 } else if ( state -> last_nav_direction > 0 ) {

 state -> auto_select_row = display_row_count - 1 ;
 } else {

 state -> auto_select_row = 2 ;
 }

 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "Auto-select row %d of %d (nav_dir=%d, has_prev=%d, has_next=%d)\n" , state -> auto_select_row , display_row_count , state -> last_nav_direction , has_prev_page , has_next_page )

 ;
 } else {
 state -> auto_select_row = - 1 ;
 }
 }

 return list ;

 cleanup_buffers :
 {
 struct Node * cleanup_node ;

 if ( row_buffer ) {
 whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 2277 ) ;
 row_buffer = ( ( void * ) 0 ) ;
 }
 if ( cell_buffer ) {
 whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 2281 ) ;
 cell_buffer = ( ( void * ) 0 ) ;
 }
 if ( col_widths ) {
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 2285 ) ;
 col_widths = ( ( void * ) 0 ) ;
 }
 if ( state ) {
 if ( state -> columns ) {
 whd_free_debug ( state -> columns , "src\\helpers\\listview_columns_api.c" , 2290 ) ;
 state -> columns = ( ( void * ) 0 ) ;
 }
 whd_free_debug ( state , "src\\helpers\\listview_columns_api.c" , 2293 ) ;
 state = ( ( void * ) 0 ) ;
 if ( out_state ) {
 * out_state = ( ( void * ) 0 ) ;
 }
 }
 if ( list ) {
 while ( ( cleanup_node = __RemHead ( SysBase , ( list ) ) ) != ( ( void * ) 0 ) ) {
 if ( cleanup_node -> ln_Name ) {
 whd_free_debug ( cleanup_node -> ln_Name , "src\\helpers\\listview_columns_api.c" , 2302 ) ;
 }
 whd_free_debug ( cleanup_node , "src\\helpers\\listview_columns_api.c" , 2304 ) ;
 }
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2306 ) ;
 list = ( ( void * ) 0 ) ;
 }
 }
 return ( ( void * ) 0 ) ;
 }










 iTidy_ListViewEntry * iTidy_GetSelectedEntry ( struct List * entry_list , LONG listview_row )
 {
 struct Node * node ;
 LONG data_index ;
 LONG current_index ;

 if ( ! entry_list ) {
 return ( ( void * ) 0 ) ;
 }


 if ( listview_row < 2 ) {
 return ( ( void * ) 0 ) ;
 }


 data_index = listview_row - 2 ;


 node = entry_list -> lh_Head ;
 current_index = 0 ;

 while ( node -> ln_Succ ) {
 if ( current_index == data_index ) {

 return ( iTidy_ListViewEntry * ) node ;
 }
 node = node -> ln_Succ ;
 current_index ++ ;
 }


 return ( ( void * ) 0 ) ;
 }











 int iTidy_GetClickedColumn ( iTidy_ListViewState * state , WORD mouse_x , WORD gadget_left )
 {
 WORD local_x ;
 int col ;

 if ( state == ( ( void * ) 0 ) ) {
 return - 1 ;
 }


 local_x = mouse_x - gadget_left ;


 for ( col = 0 ; col < state -> num_columns ; col ++ ) {
 if ( local_x >= state -> columns [ col ] . pixel_start &&
 local_x < state -> columns [ col ] . pixel_end ) {
 return col ;
 }
 }

 return - 1 ;
 }







 iTidy_ListViewClick iTidy_GetListViewClick (
 struct List * entry_list ,
 iTidy_ListViewState * state ,
 LONG listview_row ,
 WORD mouse_x ,
 WORD gadget_left )
 {
 iTidy_ListViewClick result ;


 result . entry = ( ( void * ) 0 ) ;
 result . column = - 1 ;
 result . display_value = ( ( void * ) 0 ) ;
 result . sort_key = ( ( void * ) 0 ) ;
 result . column_type = ITIDY_COLTYPE_TEXT ;


 result . entry = iTidy_GetSelectedEntry ( entry_list , listview_row ) ;


 result . column = iTidy_GetClickedColumn ( state , mouse_x , gadget_left ) ;


 if ( result . entry != ( ( void * ) 0 ) &&
 result . column >= 0 &&
 result . column < result . entry -> num_columns ) {


 result . display_value = result . entry -> display_data [ result . column ] ;


 result . sort_key = result . entry -> sort_keys [ result . column ] ;


 if ( state != ( ( void * ) 0 ) && result . column < state -> num_columns ) {
 result . column_type = state -> columns [ result . column ] . column_type ;
 }
 }

 return result ;
 }











 BOOL iTidy_HandleListViewGadgetUp (
 struct Window * window ,
 struct Gadget * gadget ,
 WORD mouse_x ,
 WORD mouse_y ,
 struct List * entry_list ,
 struct List * display_list ,
 iTidy_ListViewState * state ,
 int font_height ,
 int font_width ,
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 iTidy_ListViewEvent * out_event )
 {
 LONG selected ;
 iTidy_ListViewEventContext ctx ;
 iTidy_DisplayPaginationInfo display_info ;
 struct Node * node ;
 LONG current_row ;
 BOOL seen_prev_nav = 0 ;
 iTidy_ListViewClick legacy_click ;

 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_HandleListViewGadgetUp: state=%p, display_list=%p\n" , state , display_list ) ;


 if ( ! window || ! gadget || ! entry_list || ! display_list || ! columns || ! out_event ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_HandleListViewGadgetUp: Invalid params (window=%p, gadget=%p, entry_list=%p, display_list=%p, columns=%p, out_event=%p)\n" , window , gadget , entry_list , display_list , columns , out_event )
 ;
 if ( out_event ) {
 itidy_init_event_defaults ( out_event ) ;
 }
 return 0 ;
 }

 itidy_init_event_defaults ( out_event ) ;

 __asm_memset ( & ctx , 0 , sizeof ( ctx ) ) ;
 ctx . window = window ;
 ctx . gadget = gadget ;
 ctx . mouse_x = mouse_x ;
 ctx . mouse_y = mouse_y ;
 ctx . entry_list = entry_list ;
 ctx . display_list = display_list ;
 ctx . state = state ;
 ctx . font_height = font_height ;
 ctx . font_width = font_width ;
 ctx . columns = columns ;
 ctx . num_columns = num_columns ;
 ctx . out_event = out_event ;
 ctx . selected_row = - 1 ;



 if ( state != ( ( void * ) 0 ) ) {
 int col ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 state -> columns [ col ] . pixel_start = state -> columns [ col ] . char_start * font_width ;
 state -> columns [ col ] . pixel_end = state -> columns [ col ] . char_end * font_width ;
 }
 }


 selected = - 1 ;
 __GT_GetGadgetAttrs ( GadToolsBase , ( gadget ) , ( window ) , ( ( void * ) 0 ) , ( ( ( ( ULONG ) ( 1UL << 31 ) ) + 0x80000 ) + 54 ) , & selected , ( 0UL ) ) ;

 if ( selected < 0 ) {

 return 0 ;
 }
 ctx . selected_row = selected ;



 if ( ctx . selected_row == 0 ) {
 return itidy_handle_header_click ( & ctx ) ;
 }


 if ( ctx . selected_row == 1 ) {

 return 0 ;
 }



 if ( display_list != ( ( void * ) 0 ) && ctx . selected_row >= 2 ) {
 itidy_extract_display_pagination_info ( display_list , state , & display_info ) ;

 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "Row clicked: selected=%ld, walking display_list...\n" , ctx . selected_row ) ;

 current_row = 0 ;
 for ( node = display_list -> lh_Head ; node -> ln_Succ ; node = node -> ln_Succ ) {
 if ( current_row == ctx . selected_row ) {
 if ( itidy_handle_navigation_click ( & ctx , node ) ) {
 return 1 ;
 }
 return itidy_handle_data_row_click ( & ctx , node , seen_prev_nav , & display_info ) ;
 }

 if ( current_row >= 2 && itidy_detect_navigation_direction ( node , state ) < 0 ) {
 seen_prev_nav = 1 ;
 }

 current_row ++ ;
 }
 }



 legacy_click = iTidy_GetListViewClick (
 entry_list ,
 state ,
 ctx . selected_row ,
 ctx . mouse_x ,
 gadget -> LeftEdge
 ) ;


 if ( legacy_click . entry != ( ( void * ) 0 ) ) {

 out_event -> type = ITIDY_LV_EVENT_ROW_CLICK ;
 out_event -> entry = legacy_click . entry ;
 out_event -> column = legacy_click . column ;
 out_event -> display_value = legacy_click . display_value ;
 out_event -> sort_key = legacy_click . sort_key ;
 out_event -> column_type = legacy_click . column_type ;
 return 1 ;
 }


 return 0 ;
 }
















































 void itidy_free_listview_entries ( struct List * entry_list ,
 struct List * display_list ,
 iTidy_ListViewState * state )
 {
 struct Node * node ;
 iTidy_ListViewEntry * entry ;
 int num_columns ;
 int i ;


 num_columns = ( state && state -> num_columns > 0 ) ? state -> num_columns : 0 ;


 if ( entry_list ) {
 while ( ( node = __RemHead ( SysBase , ( entry_list ) ) ) != ( ( void * ) 0 ) ) {
 entry = ( iTidy_ListViewEntry * ) node ;


 if ( entry -> node . ln_Name ) {
 whd_free_debug ( entry -> node . ln_Name , "src\\helpers\\listview_columns_api.c" , 2647 ) ;
 entry -> node . ln_Name = ( ( void * ) 0 ) ;
 }


 if ( num_columns == 0 && entry -> num_columns > 0 ) {
 num_columns = entry -> num_columns ;
 }


 for ( i = 0 ; i < entry -> num_columns ; i ++ ) {
 if ( entry -> display_data && entry -> display_data [ i ] ) {
 whd_free_debug ( ( void * ) entry -> display_data [ i ] , "src\\helpers\\listview_columns_api.c" , 2659 ) ;
 }
 if ( entry -> sort_keys && entry -> sort_keys [ i ] ) {
 whd_free_debug ( ( void * ) entry -> sort_keys [ i ] , "src\\helpers\\listview_columns_api.c" , 2662 ) ;
 }
 }


 if ( entry -> display_data ) {
 whd_free_debug ( ( void * ) entry -> display_data , "src\\helpers\\listview_columns_api.c" , 2668 ) ;
 }
 if ( entry -> sort_keys ) {
 whd_free_debug ( ( void * ) entry -> sort_keys , "src\\helpers\\listview_columns_api.c" , 2671 ) ;
 }


 whd_free_debug ( entry , "src\\helpers\\listview_columns_api.c" , 2675 ) ;
 }
 }


 if ( display_list ) {
 iTidy_FreeFormattedList ( display_list ) ;
 }


 if ( state ) {
 iTidy_FreeListViewState ( state ) ;
 }
 }





 void iTidy_FreeFormattedList ( struct List * list )
 {
 struct Node * node , * next ;

 if ( ! list ) return ;

 node = list -> lh_Head ;
 while ( node -> ln_Succ ) {
 next = node -> ln_Succ ;

 if ( node -> ln_Name ) {
 whd_free_debug ( node -> ln_Name , "src\\helpers\\listview_columns_api.c" , 2705 ) ;
 }

 __Remove ( SysBase , ( node ) ) ;
 whd_free_debug ( node , "src\\helpers\\listview_columns_api.c" , 2709 ) ;

 node = next ;
 }

 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2714 ) ;
 }





 void iTidy_FreeListViewState ( iTidy_ListViewState * state )
 {
 if ( ! state ) return ;

 if ( state -> columns ) {
 whd_free_debug ( state -> columns , "src\\helpers\\listview_columns_api.c" , 2726 ) ;
 }

 whd_free_debug ( state , "src\\helpers\\listview_columns_api.c" , 2729 ) ;
 }





 struct List * iTidy_FormatListViewColumns_Legacy (
 iTidy_ColumnConfig * columns ,
 int num_columns ,
 const char * * * data_rows ,
 int num_rows ,
 int total_char_width )
 {
 struct List * list ;
 struct Node * node ;
 int * col_widths ;
 char * row_buffer ;
 char * cell_buffer ;
 int row , col ;
 int buffer_size ;
 int pos ;

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "iTidy_FormatListViewColumns_Legacy: num_columns=%d, num_rows=%d, width=%d\n" , num_columns , num_rows , total_char_width )
 ;

 if ( ! columns || num_columns <= 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Invalid column configuration\n" ) ;
 return ( ( void * ) 0 ) ;
 }


 list = ( struct List * ) whd_malloc_debug ( sizeof ( struct List ) , "src\\helpers\\listview_columns_api.c" , 2761 ) ;
 if ( ! list ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate list\n" ) ;
 return ( ( void * ) 0 ) ;
 }
 do { struct List * _list = ( list ) ; _list -> lh_Head = ( struct Node * ) & _list -> lh_Tail ; _list -> lh_Tail = ( ( void * ) 0 ) ; _list -> lh_TailPred = ( struct Node * ) & _list -> lh_Head ; } while ( 0 ) ;


 col_widths = ( int * ) whd_malloc_debug ( sizeof ( int ) * num_columns , "src\\helpers\\listview_columns_api.c" , 2769 ) ;
 if ( ! col_widths ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate column widths\n" ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2772 ) ;
 return ( ( void * ) 0 ) ;
 }



 int flexible_col = - 1 ;
 int separator_total = ( num_columns - 1 ) * 3 ;


 for ( col = 0 ; col < num_columns ; col ++ ) {
 int max_len = columns [ col ] . title ? __asm_strlen ( columns [ col ] . title ) : 0 ;

 if ( data_rows && num_rows > 0 ) {
 for ( row = 0 ; row < num_rows ; row ++ ) {
 if ( data_rows [ row ] && data_rows [ row ] [ col ] ) {
 int len = __asm_strlen ( data_rows [ row ] [ col ] ) ;
 if ( len > max_len ) {
 max_len = len ;
 }
 }
 }
 }

 if ( columns [ col ] . min_width > 0 && max_len < columns [ col ] . min_width ) {
 max_len = columns [ col ] . min_width ;
 }
 if ( columns [ col ] . max_width > 0 && max_len > columns [ col ] . max_width ) {
 max_len = columns [ col ] . max_width ;
 }

 col_widths [ col ] = max_len ;

 if ( columns [ col ] . flexible && flexible_col < 0 ) {
 flexible_col = col ;
 }
 }


 if ( total_char_width > 0 && flexible_col >= 0 ) {
 int fixed_width = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 if ( col != flexible_col ) {
 fixed_width += col_widths [ col ] ;
 }
 }

 int flex_width = total_char_width - separator_total - fixed_width ;
 if ( columns [ flexible_col ] . min_width > 0 && flex_width < columns [ flexible_col ] . min_width ) {
 flex_width = columns [ flexible_col ] . min_width ;
 }
 if ( columns [ flexible_col ] . max_width > 0 && flex_width > columns [ flexible_col ] . max_width ) {
 flex_width = columns [ flexible_col ] . max_width ;
 }
 col_widths [ flexible_col ] = flex_width ;
 }


 buffer_size = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 buffer_size += col_widths [ col ] ;
 }
 buffer_size += separator_total ;
 buffer_size += 10 ;


 row_buffer = ( char * ) whd_malloc_debug ( buffer_size , "src\\helpers\\listview_columns_api.c" , 2838 ) ;
 cell_buffer = ( char * ) whd_malloc_debug ( 256 , "src\\helpers\\listview_columns_api.c" , 2839 ) ;

 if ( ! row_buffer || ! cell_buffer ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "Failed to allocate formatting buffers\n" ) ;
 if ( row_buffer ) whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 2843 ) ;
 if ( cell_buffer ) whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 2844 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 2845 ) ;
 whd_free_debug ( list , "src\\helpers\\listview_columns_api.c" , 2846 ) ;
 return ( ( void * ) 0 ) ;
 }


 pos = 0 ;
 for ( col = 0 ; col < num_columns ; col ++ ) {
 format_cell ( cell_buffer , columns [ col ] . title , col_widths [ col ] , ITIDY_ALIGN_LEFT , 0 ) ;
 __asm_strcpy ( row_buffer + pos , cell_buffer ) ;
 pos += col_widths [ col ] ;

 if ( col < num_columns - 1 ) {
 __asm_strcpy ( row_buffer + pos , " | " ) ;
 pos += 3 ;
 }
 }
 row_buffer [ pos ] = '\0' ;

 node = create_display_node ( row_buffer ) ;
 if ( node ) {
 __AddTail ( SysBase , ( list ) , ( node ) ) ;
 }


 __asm_memset ( row_buffer , '-' , pos + 4 ) ;
 row_buffer [ pos + 4 ] = '\0' ;

 node = create_display_node ( row_buffer ) ;
 if ( node ) {
 __AddTail ( SysBase , ( list ) , ( node ) ) ;
 }


 if ( data_rows && num_rows > 0 ) {
 for ( row = 0 ; row < num_rows ; row ++ ) {
 pos = 0 ;

 for ( col = 0 ; col < num_columns ; col ++ ) {
 const char * cell_data = ( data_rows [ row ] && data_rows [ row ] [ col ] ) ?
 data_rows [ row ] [ col ] : "" ;

 format_cell ( cell_buffer , cell_data , col_widths [ col ] , columns [ col ] . align , columns [ col ] . is_path ) ;
 __asm_strcpy ( row_buffer + pos , cell_buffer ) ;
 pos += col_widths [ col ] ;

 if ( col < num_columns - 1 ) {
 __asm_strcpy ( row_buffer + pos , " | " ) ;
 pos += 3 ;
 }
 }
 row_buffer [ pos ] = '\0' ;

 node = create_display_node ( row_buffer ) ;
 if ( node ) {
 __AddTail ( SysBase , ( list ) , ( node ) ) ;
 }
 }
 }


 whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 2906 ) ;
 whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 2907 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 2908 ) ;

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Formatted %d columns x %d rows (legacy API)\n" , num_columns , num_rows ) ;

 return list ;
 }





 BOOL iTidy_ResortListViewByClick (
 struct List * formatted_list ,
 struct List * entry_list ,
 iTidy_ListViewState * state ,
 int mouse_x ,
 int mouse_y ,
 int header_top ,
 int header_height ,
 int gadget_left ,
 int font_width ,
 iTidy_ColumnConfig * columns )
 {
 int col , local_x , pos ;
 int clicked_col = - 1 ;
 struct Node * header_node , * separator_node , * data_node , * entry_node ;
 iTidy_ListViewEntry * entry ;
 char * row_buffer , * cell_buffer ;
 int * col_widths ;
 int buffer_size ;
 BOOL ascending ;
 struct timeval startTime , endTime , sortEndTime ;
 ULONG elapsedMicros , elapsedMillis , sortMicros = 0 , rebuildMicros = 0 ;
 int entry_count = 0 ;

 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_ResortListViewByClick: mouse_x=%d, mouse_y=%d, header_top=%d, header_height=%d, gadget_left=%d, font_width=%d\n" , mouse_x , mouse_y , header_top , header_height , gadget_left , font_width )
 ;

 if ( ! formatted_list || ! entry_list || ! state || ! columns ) {
 log_message ( LOG_GUI , LOG_LEVEL_ERROR , "iTidy_ResortListViewByClick: NULL parameter! formatted_list=%p, entry_list=%p, state=%p, columns=%p\n" , formatted_list , entry_list , state , columns )
 ;
 return 0 ;
 }


 if ( TimerBase ) {
 GetSysTime ( & startTime ) ;
 }

 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_ResortListViewByClick: num_columns=%d\n" , state -> num_columns ) ;


 if ( mouse_y < header_top || mouse_y >= header_top + header_height ) {
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_ResortListViewByClick: Click outside header Y-range (y=%d, top=%d, bottom=%d)\n" , mouse_y , header_top , header_top + header_height )
 ;
 return 0 ;
 }


 local_x = mouse_x - gadget_left ;
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_ResortListViewByClick: local_x=%d (mouse_x=%d - gadget_left=%d)\n" , local_x , mouse_x , gadget_left )
 ;


 for ( col = 0 ; col < state -> num_columns ; col ++ ) {
 state -> columns [ col ] . pixel_start = state -> columns [ col ] . char_start * font_width ;
 state -> columns [ col ] . pixel_end = state -> columns [ col ] . char_end * font_width ;
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "Column %d: char_start=%d, char_end=%d, pixel_start=%d, pixel_end=%d\n" , col , state -> columns [ col ] . char_start , state -> columns [ col ] . char_end , state -> columns [ col ] . pixel_start , state -> columns [ col ] . pixel_end )

 ;
 }


 for ( col = 0 ; col < state -> num_columns ; col ++ ) {
 if ( local_x >= state -> columns [ col ] . pixel_start && local_x < state -> columns [ col ] . pixel_end ) {
 clicked_col = col ;
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_ResortListViewByClick: Matched column %d (local_x=%d in range %d-%d)\n" , col , local_x , state -> columns [ col ] . pixel_start , state -> columns [ col ] . pixel_end )
 ;
 break ;
 }
 }

 if ( clicked_col < 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_ResortListViewByClick: Click outside all columns (local_x=%d)\n" , local_x ) ;
 return 0 ;
 }

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Column %d clicked (x=%d, local_x=%d)\n" , clicked_col , mouse_x , local_x ) ;


 if ( state -> columns [ clicked_col ] . sort_state == ITIDY_SORT_ASCENDING ) {
 state -> columns [ clicked_col ] . sort_state = ITIDY_SORT_DESCENDING ;
 ascending = 0 ;
 } else {
 state -> columns [ clicked_col ] . sort_state = ITIDY_SORT_ASCENDING ;
 ascending = 1 ;
 }


 for ( col = 0 ; col < state -> num_columns ; col ++ ) {
 if ( col != clicked_col ) {
 state -> columns [ col ] . sort_state = ITIDY_SORT_NONE ;
 }
 }


 iTidy_SortListViewEntries ( entry_list , clicked_col , columns [ clicked_col ] . sort_type , ascending ) ;


 if ( TimerBase ) {
 GetSysTime ( & sortEndTime ) ;
 sortMicros = ( ( sortEndTime . tv_secs - startTime . tv_secs ) * 1000000 ) +
 ( sortEndTime . tv_micro - startTime . tv_micro ) ;
 }


 col_widths = ( int * ) whd_malloc_debug ( sizeof ( int ) * state -> num_columns , "src\\helpers\\listview_columns_api.c" , 3024 ) ;
 if ( ! col_widths ) {
 return 0 ;
 }

 for ( col = 0 ; col < state -> num_columns ; col ++ ) {
 col_widths [ col ] = state -> columns [ col ] . char_width ;
 }


 buffer_size = 0 ;
 for ( col = 0 ; col < state -> num_columns ; col ++ ) {
 buffer_size += col_widths [ col ] ;
 }
 buffer_size += ( state -> num_columns - 1 ) * 3 ;
 buffer_size += 10 ;

 row_buffer = ( char * ) whd_malloc_debug ( buffer_size , "src\\helpers\\listview_columns_api.c" , 3041 ) ;
 cell_buffer = ( char * ) whd_malloc_debug ( 256 , "src\\helpers\\listview_columns_api.c" , 3042 ) ;

 if ( ! row_buffer || ! cell_buffer ) {
 if ( row_buffer ) whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 3045 ) ;
 if ( cell_buffer ) whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 3046 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 3047 ) ;
 return 0 ;
 }


 header_node = formatted_list -> lh_Head ;
 if ( header_node && header_node -> ln_Name ) {
 whd_free_debug ( header_node -> ln_Name , "src\\helpers\\listview_columns_api.c" , 3054 ) ;
 format_header_row ( row_buffer , cell_buffer , columns , state -> num_columns , col_widths , state ) ;
 header_node -> ln_Name = ( char * ) whd_malloc_debug ( __asm_strlen ( row_buffer ) + 1 , "src\\helpers\\listview_columns_api.c" , 3056 ) ;
 if ( header_node -> ln_Name ) {
 __asm_strcpy ( header_node -> ln_Name , row_buffer ) ;
 }
 }


 separator_node = ( header_node && header_node -> ln_Succ ) ? header_node -> ln_Succ : ( ( void * ) 0 ) ;



 if ( separator_node && separator_node -> ln_Succ ) {
 data_node = separator_node -> ln_Succ ;
 while ( data_node -> ln_Succ ) {
 struct Node * next = data_node -> ln_Succ ;
 if ( data_node -> ln_Name ) {
 whd_free_debug ( data_node -> ln_Name , "src\\helpers\\listview_columns_api.c" , 3072 ) ;
 }
 __Remove ( SysBase , ( data_node ) ) ;
 whd_free_debug ( data_node , "src\\helpers\\listview_columns_api.c" , 3075 ) ;
 data_node = next ;
 }
 }


 for ( entry_node = entry_list -> lh_Head ; entry_node -> ln_Succ ; entry_node = entry_node -> ln_Succ ) {
 entry = ( iTidy_ListViewEntry * ) entry_node ;
 pos = 0 ;

 for ( col = 0 ; col < state -> num_columns ; col ++ ) {
 const char * cell_data = ( entry -> display_data && col < entry -> num_columns && entry -> display_data [ col ] ) ?
 entry -> display_data [ col ] : "" ;

 format_cell ( cell_buffer , cell_data , col_widths [ col ] , columns [ col ] . align , columns [ col ] . is_path ) ;
 __asm_strcpy ( row_buffer + pos , cell_buffer ) ;
 pos += col_widths [ col ] ;

 if ( col < state -> num_columns - 1 ) {
 __asm_strcpy ( row_buffer + pos , " | " ) ;
 pos += 3 ;
 }
 }
 row_buffer [ pos ] = '\0' ;


 data_node = create_display_node ( row_buffer ) ;
 if ( data_node ) {
 __AddTail ( SysBase , ( formatted_list ) , ( data_node ) ) ;
 }
 }


 whd_free_debug ( row_buffer , "src\\helpers\\listview_columns_api.c" , 3108 ) ;
 whd_free_debug ( cell_buffer , "src\\helpers\\listview_columns_api.c" , 3109 ) ;
 whd_free_debug ( col_widths , "src\\helpers\\listview_columns_api.c" , 3110 ) ;


 for ( entry_node = entry_list -> lh_Head ; entry_node -> ln_Succ ; entry_node = entry_node -> ln_Succ ) {
 entry_count ++ ;
 }


 if ( TimerBase ) {
 GetSysTime ( & endTime ) ;


 elapsedMicros = ( ( endTime . tv_secs - startTime . tv_secs ) * 1000000 ) +
 ( endTime . tv_micro - startTime . tv_micro ) ;
 elapsedMillis = elapsedMicros / 1000 ;
 rebuildMicros = elapsedMicros - sortMicros ;


 if ( is_performance_logging_enabled ( ) ) {
 const char * type_str = ( columns [ clicked_col ] . sort_type == ITIDY_COLTYPE_NUMBER ) ? "NUMBER" :
 ( columns [ clicked_col ] . sort_type == ITIDY_COLTYPE_DATE ) ? "DATE" : "TEXT" ;
 const char * dir_str = ascending ? "ASC" : "DESC" ;

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "==== LISTVIEW RESORT PERFORMANCE ====\n" ) ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "iTidy_ResortListViewByClick() total time: %lu.%03lu ms\n" , elapsedMillis , elapsedMicros % 1000 )
 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "  Column: %d (%s, %s)\n" , clicked_col , type_str , dir_str ) ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "  Entries: %d, Columns: %d\n" , entry_count , state -> num_columns ) ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "  Breakdown:\n" ) ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "    Sort:      %lu.%03lu ms (%lu%%)\n" , sortMicros / 1000 , sortMicros % 1000 , elapsedMicros > 0 ? ( sortMicros * 100 / elapsedMicros ) : 0 )

 ;
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "    Rebuild:   %lu.%03lu ms (%lu%%)\n" , rebuildMicros / 1000 , rebuildMicros % 1000 , elapsedMicros > 0 ? ( rebuildMicros * 100 / elapsedMicros ) : 0 )

 ;
 if ( entry_count > 0 ) {
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "  Time per entry: %lu microseconds\n" , elapsedMicros / entry_count )
 ;
 }
 log_message ( LOG_GUI , LOG_LEVEL_INFO , "====================================\n" ) ;

 printf ( "  [TIMING] ListView resort: %lu.%03lu ms total (col %d, %s/%s, %d entries)\n" ,
 elapsedMillis , elapsedMicros % 1000 , clicked_col , type_str , dir_str , entry_count ) ;
 printf ( "           Sort: %lu.%03lu ms | Rebuild: %lu.%03lu ms\n" ,
 sortMicros / 1000 , sortMicros % 1000 ,
 rebuildMicros / 1000 , rebuildMicros % 1000 ) ;
 }
 }

 log_message ( LOG_GUI , LOG_LEVEL_INFO , "Resorted by column %d (%s)\n" , clicked_col , ascending ? "ASC" : "DESC" ) ;

 return 1 ;
 }





 BOOL iTidy_HandleListViewSort (
 struct Window * window ,
 struct Gadget * listview_gadget ,
 struct List * formatted_list ,
 struct List * entry_list ,
 iTidy_ListViewState * state ,
 int mouse_x ,
 int mouse_y ,
 int font_height ,
 int font_width ,
 iTidy_ColumnConfig * columns ,
 int num_columns )
 {
 BOOL did_sort ;
 int header_top , header_height , gadget_left ;


 if ( ! window || ! listview_gadget || ! formatted_list || ! entry_list || ! state || ! columns ) {
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_HandleListViewSort: NULL parameter\n" ) ;
 return 0 ;
 }


 header_top = listview_gadget -> TopEdge ;
 header_height = font_height ;
 gadget_left = listview_gadget -> LeftEdge ;


 if ( mouse_y < header_top || mouse_y >= header_top + header_height ) {
 return 0 ;
 }


 __SetWindowPointer ( IntuitionBase , ( window ) , ( ( ( ( ULONG ) ( 1UL << 31 ) ) + 99 ) + 0x35 ) , 1 , ( 0UL ) ) ;


 did_sort = iTidy_ResortListViewByClick (
 formatted_list ,
 entry_list ,
 state ,
 mouse_x , mouse_y ,
 header_top , header_height ,
 gadget_left ,
 font_width ,
 columns
 ) ;


 if ( did_sort ) {
 log_message ( LOG_GUI , LOG_LEVEL_DEBUG , "iTidy_HandleListViewSort: List resorted, refreshing gadget\n" ) ;


 __GT_SetGadgetAttrs ( GadToolsBase , ( listview_gadget ) , ( window ) , ( ( void * ) 0 ) , ( ( ( ( ULONG ) ( 1UL << 31 ) ) + 0x80000 ) + 6 ) , ~ 0 , ( 0UL ) )

 ;
 __GT_SetGadgetAttrs ( GadToolsBase , ( listview_gadget ) , ( window ) , ( ( void * ) 0 ) , ( ( ( ( ULONG ) ( 1UL << 31 ) ) + 0x80000 ) + 6 ) , formatted_list , ( 0UL ) )

 ;
 }


 __SetWindowPointer ( IntuitionBase , ( window ) , ( ( ( ( ULONG ) ( 1UL << 31 ) ) + 99 ) + 0x35 ) , 0 , ( 0UL ) ) ;

 return did_sort ;
 }

 
