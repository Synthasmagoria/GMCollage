#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define NEWLINE "\r\n"
#define MODULES_MAX 32

// TODO: Allocate this in dynamic memory
#define MODULE_SECTIONS_MAX 32
#define MODULE_RESOURCE_PATH_MAX 256
typedef struct
{
    unsigned sections;
    char resource_type[MODULE_SECTIONS_MAX][32];
    char path[MODULE_SECTIONS_MAX][MODULE_RESOURCE_PATH_MAX];
} Moduleconfig;

typedef enum {false, true} bool;

char* malloc_file(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (ferror(f) || !f)
    {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    unsigned size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size == 0)
    {
        return NULL;
    }

    char* buffer = malloc(size+1);
    fread(buffer, 1, size, f);
    fclose(f);
    *(buffer+size) = '\0';
    return buffer;
}

char* str_skip_line(char* buffer, char* cursor)
{
    cursor = strstr(buffer, NEWLINE);
    if (cursor == NULL || *(cursor+2) == '\0')
        return NULL;
    return cursor + 2;
}

void strcpy_from_to_nullt(char* a, char* b, char* out)
{
    unsigned i = 0;
    while (a != b)
    {
        *(out + i) = *a;
        i++;
        a++;
    }
    *(out + i) = '\0';
}

int strdist(char* from, char* to)
{
    int dist = 0;
    while (from != to && *from != '\0')
    {
        dist++;
        from++;
    }
    return dist;
}

void strpath_get_filename(char* dest, char* path)
{
    char* name_start = strrchr(path, '/');
    if (!name_start)
        name_start = path;
    else
        name_start++;
    
    char* name_end = strrchr(path, '.');
    if (!name_end)
        name_end = strchr(path, '\0');
    else if (name_start > name_end)
        return;
    
    strcpy_from_to_nullt(name_start, name_end, dest);
}

void xml_tag_extract_attr(char* tag, const char* attr_name, char* out)
{
    char* attr_cursor_begin;
    attr_cursor_begin = strchr(tag, ' ');
    if (!attr_cursor_begin)
        return;

    attr_cursor_begin++;

    char* attr_cursor_end;
    attr_cursor_end = strchr(attr_cursor_begin, '=');
    if (!attr_cursor_end)
        return;

    bool attr_names_match = true;
    for (int i = 0; *(attr_name + i) != '\0' && attr_cursor_begin + i != attr_cursor_end; i++)
        attr_names_match &= *(attr_name + i) == *(attr_cursor_begin + i);

    if (!attr_names_match)
        return;

    attr_cursor_begin = strchr(attr_cursor_end, '\"');
    if (!attr_cursor_begin)
        return;

    attr_cursor_begin++;
    attr_cursor_end = strchr(attr_cursor_begin, '\"');
    if (!attr_cursor_end)
        return;

    strcpy_from_to_nullt(attr_cursor_begin, attr_cursor_end, out);
}

// TODO: Add backups
// TODO: Add a success switch that is only true so long as all paths in the specified module were found
// TODO: Make a system for collecting errors so that they can accumulate before terminating
int main(int argc, char** argv)
{
    if (argc == 1)
    {
        printf(
            "################## GMCOLLAGE ##################\n"
            "Usage: gmcollage <project.gmx> in/out <.module/.moduleconfig> <OPTIONAL SWITCHES>\n"
            "Providing multiple module/moduleconfig paths can be done with comma separation\n"
            "\n"
            "Optional switches\n"
            "-o <DIR> | Output directory\n"
            "\n");
        return 0;
    }

    if (argc < 2)
    {
        printf("ERROR: Gamemaker project not specified\n");
        return 1;
    }

    const char* PROJECT_PATH = argv[1];

    if (argc < 3 || (strcmp(argv[2], "in") != 0 && strcmp(argv[2], "out")) != 0)
    {
        printf("ERROR: Specify whether importing or extracting module with in/out\n");
        return 1;
    }

    const bool MODULE_IN = strcmp(argv[2], "in") == 0;

    unsigned moduleconfig_number = 0;
    char* moduleconfig_paths[MODULES_MAX];
    char* output_path = "";
    char current_switch = ' ';
    char module_paths[MODULES_MAX][MODULE_RESOURCE_PATH_MAX] = {'\0'};

    for (int i = 3; i < argc; i++)
    {
        if (current_switch != ' ')
        {
            switch (current_switch)
            {
                case 'o': output_path = argv[i]; break;
                // case 'r': break; TODO: Implement file replacement switch
            }
            current_switch = ' ';
        }
        else
        {
            if (argv[i][0] == '-')
            {
                switch (argv[i][1])
                {
                    case 'o':
                    current_switch = argv[i][1];
                    break;

                    default:
                    printf("Invalid switch \"");
                    printf(argv[i]);
                    printf("\"\n");
                    break;
                }
            }
            else
            {
                if (moduleconfig_number >= MODULES_MAX)
                {
                    if (MODULE_IN)
                        printf("ERROR: Max module number exceeded");
                    else
                        printf("ERROR: Max moduleconfig number exceeded");
                    return 1;
                }

                moduleconfig_paths[moduleconfig_number] = argv[i];
                moduleconfig_number++;
            }
        }
        
    }

    if (moduleconfig_number == 0)
    {
        if (MODULE_IN)
            printf("ERROR: Specify at least one .module file to import\n");
        else
            printf("ERROR: Specify at least one .moduleconfig file to use in export\n");
        return;
    }

    char* project = malloc_file(PROJECT_PATH);
    Moduleconfig* moduleconfigs = calloc(sizeof(Moduleconfig), moduleconfig_number);
    for (int i = 0; i < moduleconfig_number; i++)
    {
        { // Extract module destination path
            strcat(module_paths[i], output_path);
            strcat(module_paths[i], "/");
            char module_name[MODULE_RESOURCE_PATH_MAX];
            strpath_get_filename(module_name, moduleconfig_paths[i]);
            strcat(module_paths[i], module_name);
            strcat(module_paths[i], ".module");
        }

        char* module_file = malloc_file(moduleconfig_paths[i]);
        Moduleconfig* moduleconfig = moduleconfigs + i;
        char* cursor = module_file;
        char* start_of_line = NULL;
        char* end_of_line = NULL;
        bool invalid_module = false;

        while (*cursor != '\0')
        {
            start_of_line = cursor;
            end_of_line = strstr(cursor, NEWLINE);
            cursor = strchr(cursor, ',');
            // TODO: moduleconfig will be deemed invalid if it doesn't end with a blank newline
            // TODO: do memory address comparisons
            if (!cursor || !end_of_line || strdist(start_of_line, cursor) >= strdist(start_of_line, end_of_line))
            {
                invalid_module = true;
                break;
            }

            strcpy_from_to_nullt(start_of_line, cursor, moduleconfig->resource_type[moduleconfig->sections]);
            cursor++;
            strcpy_from_to_nullt(cursor, end_of_line, moduleconfig->path[moduleconfig->sections]);
            moduleconfig->sections++;
            cursor = end_of_line + 2;
        }

        free(module_file);
        if (invalid_module)
        {
            return 2;
        }
    }

    
    FILE* module_streams[MODULES_MAX] = {NULL};
    for (int i = 0; i < moduleconfig_number; i++)
    {
        module_streams[i] = fopen(*(module_paths+i), "wb");
        if (ferror(module_streams[i]) || !module_streams[i])
        {
            return 1;
        }
    }

    char* cursor = project;
    char* tag_begin_cursor;
    char* tag_content_cursor;
    char* tag_name_begin_cursor;
    char* tag_name_end_cursor;

    int group_depth;
    char* group_cursor;
    bool capturing_group = false;
    unsigned module_stream_index = 0;
    char resource_type[32] = {'\0'};
    char resource_path[MODULE_RESOURCE_PATH_MAX] = {'\0'};

    char tag_content[1024];
    char tag_name[128];
    char tag_name_attr[128];

    unsigned depth = 0;

    cursor = str_skip_line(project, cursor);

    while (cursor)
    {
        *tag_content = '\0';
        *tag_name = '\0';
        *tag_name_attr = '\0';

        cursor = strchr(cursor, '<');
        if (!cursor)
            break;

        tag_begin_cursor = cursor;

        if (*(++cursor) == '\0')
            break;

        tag_content_cursor = cursor;

        bool depth_increase;
        if (*cursor == '/')
        {
            depth--;
            cursor++;
            depth_increase = false;
        }
        else
        {
            depth++;
            depth_increase = true;
        }

        cursor = strchr(cursor, '>');
        if (!cursor)
            break;

        // Look for tag attributes
        strcpy_from_to_nullt(tag_content_cursor, cursor, tag_content);
        char* tag_content_space = strchr(tag_content, ' ');
        if (tag_content_space)
            strcpy_from_to_nullt(tag_content, tag_content_space, tag_name);
        else
            strcpy_from_to_nullt(tag_content_cursor, cursor, tag_name);

        if (depth_increase)
        {
            xml_tag_extract_attr(tag_content, "name", tag_name_attr);

            // This is the depth where resource types begin
            if (depth == 2)
            {
                strcpy(resource_type, tag_name);
                *(resource_type + strlen(tag_name)) = '\0';
            }
            else if (depth == 3)
            {
                strcpy(resource_path, tag_name_attr);
            }
            else if (depth > 3)
            {
                strcat(resource_path, "/");
                strcat(resource_path, tag_name_attr);
            }

            if (!capturing_group && strcmp(resource_type, "aoeeusnthoaoeusnth") == 0)
            {
                group_cursor = tag_begin_cursor;
                group_depth = depth;
                capturing_group = true;
            }
        }
        else
        {
            if (depth == 1)
            {
                resource_type[0] = '\0';
            }
            else if (depth == 2)
            {
                resource_path[0] = '\0';
            }
            else if (depth > 2 && strcmp(resource_type, tag_name) == 0)
            {
                *(strrchr(resource_path, '/')) = '\0';
            }

            if (capturing_group && depth < group_depth)
            {
                char* group_cursor_end = cursor + 1;
                int group_size = strdist(group_cursor, group_cursor_end);

                const char module_section_start_marker[] = "### MODULE SECTION START ###" NEWLINE;
                const char module_section_header[] = "objects" "" NEWLINE;

                fwrite(module_section_start_marker, 1, strlen(module_section_start_marker), module_streams[module_stream_index]);
                fwrite(module_section_header, 1, strlen(module_section_header), module_streams[module_stream_index]);
                fwrite(group_cursor, 1, group_size + 1, module_streams[module_stream_index]);

                while (*group_cursor_end != '\0')
                {
                    *(group_cursor_end - group_size) = *group_cursor_end;
                    group_cursor_end++;
                }

                cursor = group_cursor;
                capturing_group = false;
            }
        }
    }

    for (int i = 0; i < moduleconfig_number; i++)
    {
        fclose(module_streams[i]);
    }

    {
        unsigned size = 0;
        for (char* ptr = project; *ptr != '\0'; ptr++)
            size++;
        
        FILE* f = fopen("project_modified.gmx", "wb");
        fwrite(project, 1, size, f);
        fclose(f);
    }

    free(project);

    return 0;
}
