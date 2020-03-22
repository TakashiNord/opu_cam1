//////////////////////////////////////////////////////////////////////////////
//
//  cam1.cpp
//
//  Description:
//      Contains Unigraphics entry points for the application.
//
//////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE 1

#include <stdio.h>

//  Include files
#include <uf.h>
#include <uf_exit.h>
#include <uf_ui.h>

/*
#if ! defined ( __hp9000s800 ) && ! defined ( __sgi ) && ! defined ( __sun )
# include <strstream>
  using std::ostrstream;
  using std::endl;
  using std::ends;
#else
# include <strstream.h>
#endif
#include <iostream.h>
*/

#include <uf_obj.h>

#include <uf_ui_ont.h>

#include <uf_ncgroup.h>
#include <uf_oper.h>

#include <uf_part.h>
#include <uf_param.h>
#include <uf_ugopenint.h>
#include <uf_param_indices.h>


#include "cam1.h"

#define UF_CALL(X) (report( __FILE__, __LINE__, #X, (X)))

static int report( char *file, int line, char *call, int irc)
{
  if (irc)
  {
     char    messg[133];
     printf("%s, line %d:  %s\n", file, line, call);
     (UF_get_fail_message(irc, messg)) ?
       printf("    returned a %d\n", irc) :
       printf("    returned error %d:  %s\n", irc, messg);
  }
  return(irc);
}


/*----------------------------------------------------------------------------*/
#define COUNT_GRP 300

struct GRP
{
   int     num;
   tag_t   tag;
   char    name[UF_OPER_MAX_NAME_LEN+1];
} ;

static struct GRP grp_list[COUNT_GRP] ;
int grp_list_count=0;

/* */
/*----------------------------------------------------------------------------*/
static logical cycleGenerateCb( tag_t   tag, void   *data )
{
   logical  is_group ;
   char     name[UF_OPER_MAX_NAME_LEN + 1];
   int      ecode;

   ecode = UF_NCGROUP_is_group( tag, &is_group );
   //if( is_group != TRUE ) return( TRUE );//!!!!!!!!!!!!!!!!!! важное условие

   name[0]='\0';
   ecode = UF_OBJ_ask_name(tag, name);// спросим имя обьекта
   //UF_UI_write_listing_window("\n");  UF_UI_write_listing_window(name);

   if (grp_list_count>=COUNT_GRP) {
     uc1601("Число выбранных операций/программ\n-превышает лимит в 300 единиц\n обработано только 300 операций",1);
     return( FALSE );
   }
   grp_list[grp_list_count].num=grp_list_count;
   grp_list[grp_list_count].tag=tag;
   grp_list[grp_list_count].name[0]='\0';
   sprintf(grp_list[grp_list_count].name,"%s",name);
   grp_list_count++;

   return( TRUE );
}

int _run_change ( tag_t , int , int );
int do_cam1();

//----------------------------------------------------------------------------
//  Activation Methods
//----------------------------------------------------------------------------

//  Explicit Activation
//      This entry point is used to activate the application explicitly, as in
//      "File->Execute UG/Open->User Function..."
extern "C" DllExport void ufusr( char *parm, int *returnCode, int rlen )
{
    /* Initialize the API environment */
    int errorCode = UF_initialize();

    if ( 0 == errorCode )
    {
        /* TODO: Add your application code here */
        do_cam1();

        /* Terminate the API environment */
        errorCode = UF_terminate();
    }

    /* Print out any error messages */
    PrintErrorMessage( errorCode );
}

//----------------------------------------------------------------------------
//  Utilities
//----------------------------------------------------------------------------

// Unload Handler
//     This function specifies when to unload your application from Unigraphics.
//     If your application registers a callback (from a MenuScript item or a
//     User Defined Object for example), this function MUST return
//     "UF_UNLOAD_UG_TERMINATE".
extern "C" int ufusr_ask_unload( void )
{
     /* unload immediately after application exits*/
    return ( UF_UNLOAD_IMMEDIATELY );

     /*via the unload selection dialog... */
     //return ( UF_UNLOAD_SEL_DIALOG );
     /*when UG terminates...              */
     //return ( UF_UNLOAD_UG_TERMINATE );
}

/* PrintErrorMessage
**
**     Prints error messages to standard error and the Unigraphics status
**     line. */
static void PrintErrorMessage( int errorCode )
{
    if ( 0 != errorCode )
    {
        /* Retrieve the associated error message */
        char message[133];
        UF_get_fail_message( errorCode, message );

        /* Print out the message */
        UF_UI_set_status( message );

        fprintf( stderr, "%s\n", message );
    }
}



int do_cam1()
{
/*  Variable Declarations */
    char str[133];

    tag_t       prg = NULL_TAG;
    int   i , j , count;
    int   obj_count = 0;
    tag_t *tags = NULL ;
    int set_motion_output_type , generat;

    char menu[14][38];
    int response ;
    char *mes1[]={
      "Программа предназначена для изменения параметров в операциях или программах.",
      "Проводит изменение Типа движения инструмента (линейные<->круговые<->сплайны).",
      "Для этого, Вы должны выбрать необходимые операции и нажать кнопку 'Далее..'",
      NULL
    };
    UF_UI_message_buttons_t buttons1 = { TRUE, FALSE, TRUE, "Далее....", NULL, "Отмена", 1, 0, 2 };
    char *mes2[]={
      "Производить генерацию операции после изменения?",
      NULL
    };
    UF_UI_message_buttons_t buttons2 = { TRUE, FALSE, TRUE, "Генерировать..", NULL, "Нет", 1, 0, 2 };

 /************************************************************************/

    /*
    response=0;
    UF_UI_message_dialog("Cam1.....", UF_UI_MESSAGE_INFORMATION, mes1, 3, TRUE, &buttons1, &response);
    if (response!=1) { return (0) ; }
    */

    int module_id;
    UF_ask_application_module(&module_id);
    if (UF_APP_CAM!=module_id) {
       // UF_APP_GATEWAY UF_APP_CAM UF_APP_MODELING UF_APP_NONE
       uc1601("Запуск DLL - производится из модуля обработки\n.",1);
       return (-1);
    }

    /* Ask displayed part tag */
    if (NULL_TAG==UF_PART_ask_display_part()) {
      uc1601("Cam-часть не активна.....\n Операция прервана.",1);
      return (-2);
    }

 /************************************************************************/
  printf("\n\n\nopu_cam1()\n\n");

  strcpy(&menu[0][0], "linear_only\0");
  strcpy(&menu[1][0], "cir_perp_to_taxis\0");
  strcpy(&menu[2][0], "cir_perp_parallel_to_taxis\0");
  strcpy(&menu[3][0], "nurbs\0");
  strcpy(&menu[4][0], "Help\0");

  response=3;
  while (response != 1 && response != 2  && response != 19 )
  {

   response = uc1603("Select operations for Change - Back or Cancel to terminate.",4,menu, 4+1);

   set_motion_output_type=-9999;
   switch (response)
   {
      case 5 :set_motion_output_type=-1;
      break ;
      case 6 :set_motion_output_type=0;
      break ;
      case 7 :set_motion_output_type=1;
      break ;
      case 8 :set_motion_output_type=2;
      break ;
      case 9 :
        UF_UI_open_listing_window();
        UF_UI_write_listing_window("\n#============================================================");
        UF_UI_write_listing_window("\n Автор:");
        UF_UI_write_listing_window("\n\t    ЧЕ , 2006год");
        UF_UI_write_listing_window("\n#============================================================");
        UF_UI_write_listing_window("\n# Программа предназначена для изменения параметров в операциях или программах.");
        UF_UI_write_listing_window("\n# Проводит изменение Типа движения инструмента (линейные<->круговые<->сплайны).");
        UF_UI_write_listing_window("\n# Для этого, Вы должны выбрать необходимые операции или програмы и нажать ");
        UF_UI_write_listing_window("\n# необходимый тип движения.");
        UF_UI_write_listing_window("\n#============================================================\n$$");
      //UF_UI_close_listing_window () ;
      break ;
      default : break ;
   }

   if (set_motion_output_type!=-9999) {

    /********************************************************************************/
    /* Get the number of selected operation objects. */
    if (obj_count>0) { obj_count=0 ; UF_free(tags); }
    UF_CALL( UF_UI_ONT_ask_selected_nodes(&obj_count, &tags) );
    if (obj_count<=0) { uc1601("Не выбрано операций!\n..",1); break ; }

    generat=1;
    UF_UI_message_dialog("Cam1.....", UF_UI_MESSAGE_QUESTION, mes2, 1, TRUE, &buttons2, &generat);
    if (generat==2) { generat=0; }

    UF_UI_toggle_stoplight(1);

    count=0;
    for(i=0;i<obj_count;i++)
    {
       prg = tags[i]; // идентификатор объекта

       grp_list_count=0;// заполняем структуру
       UF_CALL(  UF_NCGROUP_cycle_members( prg, cycleGenerateCb,NULL ) );

       // эти изменения мы могли проводить в cycleGenerateCb
       // но я оставил так как было.
       if (grp_list_count==0) {
         count+=_run_change ( prg , set_motion_output_type , generat );
       } else for (j=0;j<grp_list_count;j++) {
                   count+=_run_change ( grp_list[j].tag , set_motion_output_type , generat );
              }

    }

    str[0]='\0';
    sprintf(str,"Ready\nObject(s) change =%d  ( of %d )",count,obj_count);
    //sprintf(str,"Изменено операций=%d \n программа завершена.",count);

    if (obj_count>0) { obj_count=0 ; UF_free(tags); }

    UF_UI_ONT_refresh();
    UF_UI_toggle_stoplight(0);

    uc1601(str,1);

   }

 } // end while uc1603

 //UF_DISP_refresh ();
 //UF_UI_ONT_refresh();

 printf("\n\n\nopu_cam1()---end\n\n");

 return (0);
}


/*----------------------------------------------------------------------------*/
int _run_change ( tag_t prg , int set_motion_output_type , int generat)
{
  int   cnt ;
  int   motion_output_type ;
//  char  prog_name[UF_OPER_MAX_NAME_LEN+1];
  int   type, subtype ;
  int   motion_type ;
  int   err ;
  logical generated;

  cnt=0 ;

  /* type =               subtype =
       программа=121              160
       операция =100              110 */
  UF_CALL( UF_OBJ_ask_type_and_subtype (prg, &type, &subtype ) );
  if (type!=UF_machining_operation_type) { return(cnt) ; }

//  prog_name[0]='\0';
  //UF_OBJ_ask_name(prg, prog_name);// спросим имя обьекта
//  UF_CALL( UF_OPER_ask_name_from_tag(prg, prog_name) );

  /*******************************************************/

  switch (subtype) {
  	case 510:case 520:case 530:case 540:case 550:case 560:
    case 1200:case 1400:
  	  motion_type = UF_PARAM_TURN_MACH_MOTION ;
  	  if (set_motion_output_type==-1 || set_motion_output_type==2) set_motion_output_type=0 ;
  	    else if (set_motion_output_type==0 || set_motion_output_type==1) set_motion_output_type=1 ;
  	break ;
  	default :
  	  motion_type = UF_PARAM_MOTION_OUTPUT_TYPE ;
  	break ;
  }

  err = UF_CALL( UF_PARAM_ask_int_value(prg,motion_type,&motion_output_type) );

  // изменяем, если в операции другой тип
  if (motion_output_type!=set_motion_output_type && err==0){

    motion_output_type=set_motion_output_type;

    UF_CALL( UF_PARAM_set_int_value(prg,motion_type,motion_output_type) );

    if (generat==1) { UF_CALL( UF_PARAM_generate (prg,&generated ) ); }

    cnt=1;
  }

  return(cnt);
}


