#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define NEWLINE "\r\n"

// TODO: Allocate this in dynamic memory
#define MODULE_SECTIONS_MAX 32
typedef struct
{
    char resource_type[MODULE_SECTIONS_MAX][32];
    char path[MODULE_SECTIONS_MAX][256];
} Module;

typedef enum {false, true} bool;

char* str_skip_line(char* buffer, char* cursor)
{
    cursor = strstr(buffer, NEWLINE);
    if (cursor == NULL || *(cursor+2) == '\0')
        return NULL;
    return cursor + 2;
}

void str_copy_from_to_nullt(char* a, char* b, char* out)
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

void str_tag_extract_attr(char* tag, const char* attr_name, char* out)
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

    str_copy_from_to_nullt(attr_cursor_begin, attr_cursor_end, out);
}

// TODO: Add a backups
// TODO: Add a success switch that is only true so long as all paths in the specified module were found
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Gamemaker project not specified");
        return 0;
    }

    const char* PROJECT_PATH = argv[1];

    char* project;
    {
        FILE* f = fopen(PROJECT_PATH, "rb");
        if (!f)
        {
            return 1;
        }

        fseek(f, 0, SEEK_END);
        unsigned length = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (!length)
        {
            return 1;
        }

        project = malloc(length+1);
        if (!project)
        {
            return 1;
        }
        *(project+length) = '\0';

        fread(project, sizeof(char), length, f);
        if (ferror(f))
        {
            return 1;
        }

        fclose(f);
    }

    FILE* module_out = fopen("module.txt", "wb");
    if (ferror(module_out))
    {
        return 1;
    }

    char* resource_type = "objects";
    char* cursor = project;
    char* tag_begin_cursor;
    char* tag_content_cursor;
    char* tag_name_begin_cursor;
    char* tag_name_end_cursor;

    int group_depth;
    char* group_cursor;
    bool capturing_group = false;

    char tag_content[1024];
    char tag_name[128];
    char attr_name[128];
    unsigned depth = 0;

    cursor = str_skip_line(project, cursor);

    while (cursor)
    {
        *tag_content = '\0';
        *tag_name = '\0';
        *attr_name = '\0';

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

        if (depth_increase)
        {
            // Look for tag attributes
            str_copy_from_to_nullt(tag_content_cursor, cursor, tag_content);

            char* tag_content_space = strchr(tag_content, ' ');
            if (tag_content_space)
                str_copy_from_to_nullt(tag_content, tag_content_space, tag_name);
            else
                str_copy_from_to_nullt(tag_content_cursor, cursor, tag_name);

            str_tag_extract_attr(tag_content, "name", attr_name);

            if (!capturing_group && strcmp(tag_name, resource_type) == 0)
            {
                group_cursor = tag_begin_cursor;
                group_depth = depth;
                capturing_group = true;
            }
        }
        else
        {
            if (capturing_group && depth < group_depth)
            {
                char* group_cursor_end = cursor + 1;
                int group_size = 0;
                for (char* ptr = group_cursor; ptr != group_cursor_end; ptr++)
                    group_size++;

                const char module_section_start_marker[] = "### MODULE SECTION START ###" NEWLINE;
                const char module_section_header[] = "objects" "" NEWLINE;

                fwrite(module_section_start_marker, 1, strlen(module_section_start_marker), module_out);
                fwrite(module_section_header, 1, strlen(module_section_header), module_out);
                fwrite(group_cursor, 1, group_size + 1, module_out);

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

    fclose(module_out);

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
