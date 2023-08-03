#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define NEWLINE "\r\n"

typedef enum {false, true} bool;

// Various utility functions
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

char* str_skip_line(char* cursor)
{
    cursor = strstr(cursor, NEWLINE);
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

int strcnt(char* str, char chr)
{
    int count = 0;
    while (*str != '\0')
    {
        count += *str == chr;
        str++;
    }
    return count;
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
    *out = '\0';
    char* attr_cursor_begin;
    attr_cursor_begin = strchr(tag, ' ');
    if (!attr_cursor_begin)
        return;

    attr_cursor_begin = strstr(attr_cursor_begin, attr_name);
    if (!attr_cursor_begin)
        return;

    char* attr_cursor_end = strchr(attr_cursor_begin, '=');
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

// Modules
#define MODULES_MAX 32
#define MODULE_SECTIONS_MAX 32
#define MODULE_RESOURCE_TYPE_MAX 32
#define MODULE_RESOURCE_PATH_MAX 256
// TODO: No need to distinguish between resource type and path
typedef struct
{
    unsigned section_number;
    char resource_type[MODULE_SECTIONS_MAX][MODULE_RESOURCE_TYPE_MAX];
    char resource_path[MODULE_SECTIONS_MAX][MODULE_RESOURCE_PATH_MAX];
    char* buffer; // unused in OUT
    bool written_sections[MODULE_SECTIONS_MAX];
} Module;

// Variables used exclusively when extracting a module from a project.gmx file
typedef struct
{
    int depth;
    char* cursor;
    Module* moduleconfig;
    int moduleconfig_section_index;
    FILE* module_stream;
} ModuleSectionStart;

const char* const MODULE_SECTION_START_MARKER = "### MODULE SECTION START ###" NEWLINE;

int module_find_section(Module* moduleconfig, const char* resource_type, const char* path)
{
    int section = -1;
    for (int i = 0; i < moduleconfig->section_number; i++)
    {
        if (strcmp(resource_type, moduleconfig->resource_type[i]) == 0 &&
            strcmp(path, moduleconfig->resource_path[i]) == 0)
        {
            section = i;
            break;
        }
    }
    return section;
}



bool module_init_from_buffer(Module* module, char* buffer)
{
    module->buffer = buffer;
    module->section_number = 0;
    for (int i = 0; i < MODULE_SECTIONS_MAX; i++)
        module->written_sections[i] = false;

    char* cursor = module->buffer;
    do {
        cursor = strstr(cursor, MODULE_SECTION_START_MARKER);
        if (!cursor)
        {
            break;
        }
        cursor = str_skip_line(cursor);
        if (!cursor)
        {
            return false;
        }
        char* param_separator = strchr(cursor, ',');
        if (!param_separator)
        {
            return false;
        }
        char* end_of_params = strstr(cursor, NEWLINE);
        if (!end_of_params)
        {
            return false;
        }
        if (param_separator + 1 > end_of_params)
        {
            return false;
        }
        strcpy_from_to_nullt(cursor, param_separator, module->resource_type[module->section_number]);
        strcpy_from_to_nullt(param_separator + 1, end_of_params, module->resource_path[module->section_number]);
        module->section_number++;
    } while (cursor);

    return true;
}

bool module_init_from_config(Module* module, char* buffer)
{
    char* cursor = buffer;
    char* start_of_line = NULL;
    char* end_of_line = NULL;

    module->section_number = 0;
    module->buffer = NULL;
    for (int i = 0; i < MODULE_SECTIONS_MAX; i++)
        module->written_sections[i] = 0;

    while (*cursor != '\0')
    {
        start_of_line = cursor;
        end_of_line = strstr(cursor, NEWLINE);
        cursor = strchr(cursor, ',');
        // TODO: moduleconfig will be deemed invalid if it doesn't end with a blank newline
        if (!cursor || !end_of_line || cursor > end_of_line)
            return false;

        strcpy_from_to_nullt(start_of_line, cursor, module->resource_type[module->section_number]);
        cursor++;
        strcpy_from_to_nullt(cursor, end_of_line, module->resource_path[module->section_number]);
        module->section_number++;
        cursor = end_of_line + 2;
    }

    return true;
}

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
            "-f       | Overwrites the existing project instead of creating a new one"
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

    const bool MODULE_OUT = strcmp(argv[2], "out") == 0;

    unsigned file_number = 0;
    char* file_paths[MODULES_MAX] = {'\0'};
    char* output_path = NULL;
    char current_switch = ' ';

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
                if (file_number >= MODULES_MAX)
                {
                    if (!MODULE_OUT)
                        printf("ERROR: Max module number exceeded");
                    else
                        printf("ERROR: Max moduleconfig number exceeded");
                    return 1;
                }

                file_paths[file_number] = argv[i];
                file_number++;
            }
        }

    }

    if (file_number == 0)
    {
        if (!MODULE_OUT)
            printf("ERROR: Specify at least one .module file to import\n");
        else
            printf("ERROR: Specify at least one .moduleconfig file to use in export\n");
        return;
    }

    // Out / in initialization
    char* project = malloc_file(PROJECT_PATH);
    Module* modules = calloc(sizeof(Module), file_number);
    FILE* module_streams[MODULES_MAX] = {NULL};

    if (MODULE_OUT)
    {
        char module_paths[MODULES_MAX][MODULE_RESOURCE_PATH_MAX] = {'\0'};

        for (int i = 0; i < file_number; i++)
        {
            { // Extract module destination path
                strcat(module_paths[i], output_path);
                strcat(module_paths[i], "/");
                char module_name[MODULE_RESOURCE_PATH_MAX];
                strpath_get_filename(module_name, file_paths[i]);
                strcat(module_paths[i], module_name);
                strcat(module_paths[i], ".module");
            }

            char* module_file = malloc_file(file_paths[i]);
            Module* moduleconfig = modules + i;
            int invalid_modules = 0;

            for (int i = 0; i < file_number; i++)
                invalid_modules += (!module_init_from_config(modules + i, malloc_file(file_paths[i])));

            free(module_file);
            if (invalid_modules)
            {
                return 2;
            }
        }

        for (int i = 0; i < file_number; i++)
        {
            module_streams[i] = fopen(*(module_paths+i), "wb");
            if (ferror(module_streams[i]) || !module_streams[i])
            {
                return 1;
            }
        }
    }
    else
    {
        int invalid_module_number = 0;
        int non_existent_file_number = 0;
        
        for (int i = 0; i < file_number; i++)
        {
            char* module_buffer = malloc_file(file_paths[i]);
            if (module_buffer)
            {
                if (!module_init_from_buffer(modules + i, module_buffer))
                {
                    invalid_module_number++;
                    printf("ERROR: Invalid module file at '");
                    printf(file_paths[i]);
                    printf("'\n");
                }
            }
            else
            {
                non_existent_file_number++;
                printf("Couldn't open module for reading at '");
                printf(file_paths[i]);
                printf("'\n");
                return 1;
            }
        }

        if (invalid_module_number || non_existent_file_number)
        {
            return 2;
        }
    }

    FILE* project_out = fopen("project_modified.gmx", "wb");
    char* cursor = project;
    unsigned depth = 0;

    // OUT vars
    bool section_capturing = false;
    ModuleSectionStart section;
    section.depth = -1;
    section.cursor = NULL;
    section.module_stream = NULL;
    section.moduleconfig = NULL;
    section.moduleconfig_section_index = -1;

    // IN vars
    char* write_cursor = cursor;

    char current_resource_type[MODULE_RESOURCE_TYPE_MAX] = {'\0'};
    char current_resource_path[MODULE_RESOURCE_PATH_MAX] = {'\0'};

    cursor = str_skip_line(cursor);

    while (cursor)
    {
        cursor = strchr(cursor, '<');
        if (!cursor)
            break;

        char* tag_begin_cursor = cursor;

        if (*(++cursor) == '\0')
            break;

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

        char* tag_content_cursor = cursor;

        cursor = strchr(cursor, '>');
        if (!cursor)
            break;

        // Look for tag attributes
        char tag_content[1024] = {'\0'};
        char tag_name[128] = {'\0'};
        char tag_name_attr[128] = {'\0'};

        strcpy_from_to_nullt(tag_content_cursor, cursor, tag_content);
        char* tag_content_space = strchr(tag_content, ' ');
        if (tag_content_space)
            strcpy_from_to_nullt(tag_content, tag_content_space, tag_name);
        else
            strcpy_from_to_nullt(tag_content_cursor, cursor, tag_name);

        xml_tag_extract_attr(tag_content, "name", tag_name_attr);

        cursor++;

        // TODO: See if some vars can be moved closer to here for the sake of clarity
        if (depth_increase)
        {
            /*
                NOTE: Currently the program is treating every tag starting at depth 2
                as a root folder for resources. It therefore expects every child tag that is named
                the same as the resource tag to have a name attribute.
                The program will fail in one way or another if this is not the case
            */
            if (depth == 2)
            {
                strcpy(current_resource_type, tag_name);
            }
            else if (depth == 3)
            {
                if (strcmp(tag_name, current_resource_type) == 0)
                    strcpy(current_resource_path, tag_name_attr);
            }
            else if (depth > 3)
            {
                if (strcmp(tag_name, current_resource_type) == 0)
                {
                    strcat(current_resource_path, "/");
                    strcat(current_resource_path, tag_name_attr);
                }
            }

            if (MODULE_OUT)
            {
                // TODO: Abort if detecting module within module
                if (!section_capturing)
                {
                    int section_index = -1;
                    int moduleconfig_index = 0;
                    while (moduleconfig_index < file_number)
                    {
                        section_index = module_find_section(modules + moduleconfig_index, current_resource_type, current_resource_path);
                        if (section_index != -1)
                            break;
                        moduleconfig_index++;
                    }

                    if (section_index != -1)
                    {
                        section.cursor = tag_begin_cursor;
                        section.depth = depth;
                        section.module_stream = module_streams[moduleconfig_index];
                        section.moduleconfig = modules + moduleconfig_index;
                        section.moduleconfig_section_index = section_index;
                        section_capturing = true;
                    }
                }
            }
            else
            {
                if (depth > 1)
                {
                    int section_index = -1;
                    int module_index = 0;

                    for (int i = 0; i < file_number; i++)
                    {
                        Module* module = modules + i;
                        for (int ii = 0; ii < module->section_number; ii++)
                        {
                            // Check if in directory corresponding with module section
                            if (module->written_sections[ii])
                                continue;

                            if (strcmp(current_resource_type, module->resource_type[ii]) != 0)
                                continue;

                            if (strcnt(module->resource_path[ii], '/') > 0)
                            {
                                char module_resource_folder[MODULE_RESOURCE_PATH_MAX];
                                strcpy_from_to_nullt(
                                    module->resource_path[ii],
                                    strrchr(module->resource_path[ii], '/'),
                                    module_resource_folder);
                                if (strcmp(module_resource_folder, current_resource_path) != 0)
                                    continue;
                            }

                            // Insert module into project file


                            char* module_start_cursor = str_skip_line(module->buffer);
                            for (int iii = 0; iii < ii; iii++)
                            {
                                module_start_cursor = strstr(module_start_cursor, MODULE_SECTION_START_MARKER);
                                module_start_cursor = str_skip_line(module_start_cursor);
                            }
                            module_start_cursor = str_skip_line(module_start_cursor);

                            char* module_end_cursor;
                            if (ii + 1 < module->section_number)
                                module_end_cursor = strstr(module_start_cursor, MODULE_SECTION_START_MARKER);
                            else
                                module_end_cursor = strchr(module_start_cursor, '\0');

                            write_cursor += fwrite(write_cursor, 1, cursor - write_cursor, project_out);
                            fwrite(NEWLINE, 1, strlen(NEWLINE), project_out);
                            fwrite(module_start_cursor, 1, module_end_cursor - module_start_cursor, project_out);
                            // TODO: Notify the user of unsuccessful module copies
                            module->written_sections[ii] = true;
                        }
                    }
                }
            }
        }
        else
        {
            if (MODULE_OUT)
            {
                if (section_capturing && depth < section.depth)
                {
                    const char* path = section.moduleconfig->resource_path[section.moduleconfig_section_index];
                    const char* type = section.moduleconfig->resource_type[section.moduleconfig_section_index];

                    fwrite(MODULE_SECTION_START_MARKER, 1, strlen(MODULE_SECTION_START_MARKER), section.module_stream);
                    fwrite(type, 1, strlen(type), section.module_stream);
                    fwrite(",", 1, 1, section.module_stream);
                    fwrite(path, 1, strlen(path), section.module_stream);
                    fwrite(NEWLINE, 1, strlen(NEWLINE), section.module_stream);
                    for (int i = 0; i < depth; i++)
                        fwrite("\t", 1, 1, section.module_stream);
                    int section_size = cursor - section.cursor;
                    fwrite(section.cursor, 1, section_size, section.module_stream);

                    int whitespace_padding = strchr(cursor, '<') - cursor;
                    char* section_cursor_end = cursor + whitespace_padding;
                    section_size += whitespace_padding;

                    while (*section_cursor_end != '\0')
                    {
                        *(section_cursor_end - section_size) = *section_cursor_end;
                        section_cursor_end++;
                    }
                    *(section_cursor_end - section_size) = *section_cursor_end;

                    cursor = section.cursor;
                    section_capturing = false;
                    section.moduleconfig->written_sections[section.moduleconfig_section_index] = true;
                }
            }

            if (depth == 1)
                current_resource_type[0] = '\0';
            else if (depth == 2)
                current_resource_path[0] = '\0';
            else if (depth > 2 && strcmp(current_resource_type, tag_name) == 0)
                *(strrchr(current_resource_path, '/')) = '\0';
        }
    }

    if (MODULE_OUT)
    {
        for (int i = 0; i < file_number; i++)
        {
            Module* module = modules + i;
            for (int ii = 0; ii < module->section_number; ii++)
            {
                if (!module->written_sections[ii])
                {
                    printf("WARNING: Couldn't find folder '");
                    printf(module->resource_path[ii]);
                    printf(",");
                    printf(module->resource_path[ii]);
                    printf("' in ");
                    printf(PROJECT_PATH);
                    printf("\n");
                }
            }
        }

        for (int i = 0; i < file_number; i++)
            fclose(module_streams[i]);
        fwrite(project, 1, strlen(project), project_out);
    }
    else
    {
        for (int i = 0; i < file_number; i++)
            free(modules[i].buffer);
        // TODO: Check if this will actually free all the memory
        free(modules);
        fwrite(write_cursor, 1, strchr(project, '\0') - write_cursor, project_out);
    }

    fclose(project_out);
    free(project);

    return 0;
}

