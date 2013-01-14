/******************************************************************************
 *                                                                            *
 *      gtk-wrap                                                              *
 *                                                                            *
 *      Author: Michal Jamry                                                  *
 *                                                                            *
 *      License: LGPL                                                         *
 *                                                                            *
 *                                                                            *
 *****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <unistd.h>


#define STRING_SIZE 128

//"parsing" patterns for glade xml
//do wypierdolenia!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define OBJECT_TAG "<object class=\""
#define SIGNAL_TAG "<signal name=\""
#define ID_TAG "\" id=\""
#define HANDLER_TAG "\" handler=\""

short VERBOSE = 0;
short RUNNING = 1;
char *appname;
char **SIG_HANDLERS;
GtkBuilder *builder;
char *fpipeout = NULL;
char *fpipein = NULL;



void on_window_destroy(GObject *object, gpointer user_data){
    if(VERBOSE)
        fprintf(stderr, "Caught destroy signal from main widget!\nQuitting...\n");

    RUNNING = 0;
    gtk_main_quit();
}


void signal_handler(gpointer user_data, GObject *object){
    char *jo = (char *)user_data;
    fprintf(stdout, "%s\n", jo);
    fflush(stdout);
}


void *reader_loop(void* wojd){

    mkfifo(fpipeout, S_IRWXU);
    FILE *fileout = fopen(fpipeout, "a+");
    if(!fileout){
        fprintf(stderr, "Error opening pipe %s !\n", fpipeout);
        pthread_exit(NULL);
    }


    mkfifo(fpipein, S_IRWXU);
    FILE *filein = fopen(fpipein, "r+");    
    if(!filein){
        fprintf(stderr, "Error opening pipe %s !\n", fpipein);
        pthread_exit(NULL);
    }

    if(VERBOSE)
        fprintf(stderr, "Using pipes out:%s in:%s\n", fpipeout, fpipein);


    char input[1024]; 
    char *operanda = NULL;
    char *object = NULL;
    char *command = NULL;


    while(RUNNING){
        fgets(input, 1024, filein);

        if(!RUNNING)
            break; 

        object = input;
        command = input;
        operanda = input;

        while(*command && *command != ' ')
            command++;

        *command = '\0';
        operanda = ++command;

        while(*operanda && *operanda != ' ')
            operanda++;
        *operanda++ = '\0';

        if(VERBOSE)
            fprintf(stderr, "Command:> %s %s %s\n", object, command, operanda);
  
        GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(builder, object));
        
 
        //window set title
        if(!strcmp(command, "set_window_title")){
            gtk_window_set_title(GTK_WINDOW(widget), operanda);
        } else
        
        //window show
        if(!strcmp(command, "show")){
            gtk_widget_show(widget);
        } else
        
        //window hide
        if(!strcmp(command, "hide")){
            gtk_widget_hide(widget);
        } else


        //textview set text
        if(!strcmp(command, "set_textview_text")){
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)), operanda, -1); 
        } else

        //textview get text 
        if(!strcmp(command, "get_textview_text")){
            GtkTextIter a, b;
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)); 
            gtk_text_buffer_get_iter_at_offset(buffer, &a, 0);
            gtk_text_buffer_get_iter_at_offset(buffer, &b, -1);
            gchar* mtext = gtk_text_buffer_get_text(buffer, &a, &b, FALSE);
            fprintf(fileout, "%s\n", mtext);  
            fflush(fileout);
        } else
        //

        //spinner activate/deactivate
        if(!strcmp(command, "spinner_start")){
            gtk_spinner_start(GTK_SPINNER(widget)); 
        } else

        if(!strcmp(command, "spinner_stop")){
            gtk_spinner_stop(GTK_SPINNER(widget)); 
        } else

        //label set/get
        if(!strcmp(command, "set_label_text")){
            gtk_label_set_text(GTK_LABEL(widget), operanda);
        } else

        //set button label
        if(!strcmp(command, "set_button_label")){
            gtk_button_set_label(GTK_BUTTON(widget), operanda);
        } else

        //entrytext set/get
        if(!strcmp(command, "get_entry_text")){
            gchar* mtext = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
            fprintf(fileout, "%s\n", mtext);  
            fflush(fileout);
        } else
        
        if(!strcmp(command, "set_entry_text")){
            gtk_entry_set_text(GTK_ENTRY(widget), operanda);
        } else


        //combobox add options, get/set selected 
        if(!strcmp(command, "set_combobox_items")){
            //GtkTreeModel *tree_model;
            //gtk_combo_box_model_set(GTK_COMBO_BOX(widget), tree_model);

        } else
        if(!strcmp(command, "get_selected_combobox_item")){
            fprintf(fileout, "%d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));  
            fflush(fileout);
        } else

        //image set image TODO doesn't work
        if(!strcmp(command, "set_image")){
            gtk_image_set_from_file(GTK_IMAGE(widget), operanda);
            gtk_widget_show(widget);
        } else

        //progressbar set, show/hide
        if(!strcmp(command, "set_progressbar")){

        } else

        //togglebutton istoggled //toggle, check, radio button 
        if(!strcmp(command, "get_button_state")){
            if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
                fprintf(fileout, "1\n");
            else
                fprintf(fileout, "0\n");
            fflush(fileout);
        }

    }

    fclose(filein);
    fflush(fileout);
    fclose(fileout);
    pthread_exit(NULL);
}

//Adding signals handled in glade file
//TODO: make less dumb, replace by real xml parser
void auto_add_signals(char *filename, GtkBuilder *builder){

    FILE *file = fopen(filename, "r");
    char line[STRING_SIZE];
    char objclass[STRING_SIZE];
    char objname[STRING_SIZE];
    char signame[STRING_SIZE];
    char sighandler[STRING_SIZE];
    char *a;
    int hand_count = 0;
    

    SIG_HANDLERS = (char**)calloc(50, sizeof(char*));
    if(!SIG_HANDLERS){
        fprintf(stderr, "Error allocating memory: SIG_HANDLERS!\n");
        return;
    }
    
    if(!file){
        fprintf(stderr, "Couldn't open file %s, no signals will be auto-handled!\n", filename);
        return;
    }
        while(!feof(file)){

            fgets(line, STRING_SIZE, file);
            
                if((a = strstr(line, OBJECT_TAG)) != NULL){

                    a += strlen(OBJECT_TAG);  
                
                    int i = 0;
                    for (; i < STRING_SIZE - 1 && *a != '\"'; i++)
                        objclass[i] = *a++;

                    objclass[i] = '\0';

                    a += strlen(ID_TAG);

                    for ( i = 0; i < STRING_SIZE - 1 && *a != '\"'; i++)
                        objname[i] = *a++;

                    objname[i] = '\0';
                
                    continue;        
                }            

                if ((a = strstr(line, SIGNAL_TAG)) != NULL)
                {
                    a += strlen(SIGNAL_TAG);

                    int i = 0;
                    for (; i < STRING_SIZE - 1 && *a != '\"'; i++)
                        signame[i] = *a++;
                    
                    signame[i] = '\0';

                    a += strlen(HANDLER_TAG);

                    for(i = 0; i < STRING_SIZE - 1 && *a != '\"'; i++)
                        sighandler[i] = *a++;

                    sighandler[i] = '\0';

                    if(VERBOSE)
                        fprintf(stderr, "Found signal \"%s\", handled by \"%s\" in object \"%s\" (%s)\n", signame, sighandler, objname, objclass);

                    SIG_HANDLERS[hand_count] = (char*)calloc(strlen(sighandler) + 1, sizeof(char));
                    strncpy(SIG_HANDLERS[hand_count], sighandler, strlen(sighandler));

                    GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object( builder, objname));
                    g_signal_connect_swapped(widget, signame, G_CALLBACK(signal_handler), SIG_HANDLERS[hand_count]);

                    hand_count++;
                    continue;
                  }
         }

        SIG_HANDLERS[hand_count] = NULL;

   fclose(file); 
}

void usage(){
    fprintf(stderr, "Usage:\n%s \n\nOptions:\n-f project.glade\n-m OBJECTNAME \t\t Set object as a main window. Default \"window1\".\n-v \t\t\t Be more verbose.\n-i INPIPENAME \t\t Use pipe for comands instead of standard input.\n-o OUTPIPE \t\t Use pipe for commands output.\n", appname);
    
    exit(1);
}

int main(int argc, char *argv[])
{
    char *filename = NULL;
    char *main_object = (char*)"window1";
    int argn;

    appname = argv[0];

    for (argn = 1; argn < argc; argn++)
    {
        if(strlen(argv[argn]) < 2 )
            continue;

        if(argv[argn][0] == '-'){
            switch(argv[argn][1]){

                //load object as a main widget, default window1
                case 'm' :
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        main_object = argv[++argn];
                    continue;

                //verbose
                case 'v':
                    VERBOSE = 1;
                    continue;

                //command output pipe
                case 'o':
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        fpipeout = argv[++argn];
                    continue;

                //command input pipe
                case 'i':
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        fpipein = argv[++argn];
                    continue;
                
                //read ui from GtkBuilder(Glade) file
                case 'f':
                    if(filename != NULL)
                        usage();
                    
                    if((argc - argn) > 0 && strlen(argv[argn+1]) > 0)
                        filename = argv[++argn];
                    continue;

                default:
                    usage();
                    break;
            }
        }
        break;   
    }

    if(!filename)
        usage();

    if(VERBOSE)
        fprintf(stderr, "Loading widget \"%s\" as a main window.\n", main_object);

    GtkWidget *window;
    GError *error = NULL;

    argv = &argv[argn];
    argc -= argn;

    gtk_init(&argc, &argv);    

    builder = gtk_builder_new();

    if (!gtk_builder_add_from_file(builder, filename, &error))
    {
        fprintf(stderr, "Error occured while loading UI!\n");
        fprintf(stderr, "Message: %s\n", error->message);
        g_free(error);
        return 1;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, main_object));

    //Adding default closing signal 
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(on_window_destroy), NULL);    

    //Adding other signals 
    auto_add_signals(filename, builder);

    gtk_widget_show(window);

    //starting command reader
    pthread_t thread;

    if(fpipeout)
        pthread_create(&thread, NULL, reader_loop, NULL);

    gtk_main();
    
    RUNNING = 0;

    if(fpipeout)
        pthread_cancel(thread);

    g_object_unref(G_OBJECT(builder));

    if(VERBOSE)
        fprintf(stderr, "Cleaning...\n");

    char **tmp = SIG_HANDLERS;
    while(*tmp)
        free(*tmp++);
    free(SIG_HANDLERS);

    unlink(fpipeout);
    unlink(fpipein);

    return 0;
}
