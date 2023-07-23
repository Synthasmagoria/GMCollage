#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define READ_SPAN 16384

typedef enum {false, true} bool;

char* str_skip_line(char* buffer, char* cursor)
{
    cursor = strstr(buffer, "\r\n");
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

int main()
{
    FILE* f = fopen("project.gmx", "rb");
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

    char* buffer = malloc(length+1);
    if (!buffer)
    {
        return 1;
    }
    *(buffer+length) = '\0';

    fread(buffer, sizeof(char), length, f);
    if (ferror(f))
    {
        return 1;
    }

    fclose(f);

    // Test string for later
    //buffer = 
    //    "<!-- lol comment-->\r\n"
    //    "<assets>\r\n"
    //        "<object name=\"named\">hellllooooo</object>\r\n"
    //    "</assets>\0";
    char* cursor = buffer;
    char* tag_content_cursor;
    char* tag_name_begin_cursor;
    char* tag_name_end_cursor;
    char tag_content[1024];
    char tag_name[128];
    char attr_name[128];
    unsigned depth = 0;

    cursor = str_skip_line(buffer, cursor);

    while (cursor)
    {
        *tag_content = '\0';
        *tag_name = '\0';
        *attr_name = '\0';

        cursor = strchr(cursor, '<');
        if (!cursor)
            break;
        

        if (*(++cursor) == '\0')
            break;

        tag_content_cursor = cursor;

        if (*cursor == '/')
        {
            depth--;
            cursor++;
        }
        else
        {
            depth++;
        }

        cursor = strchr(cursor, '>');
        if (!cursor)
            break;
        
        str_copy_from_to_nullt(tag_content_cursor, cursor, tag_content);

        char* tag_content_space = strchr(tag_content, ' ');
        if (tag_content_space)
            str_copy_from_to_nullt(tag_content, tag_content_space, tag_name);
        else
            str_copy_from_to_nullt(tag_content_cursor, cursor, tag_name);

        str_tag_extract_attr(tag_content, "name", attr_name);
        
        if (*attr_name != '\0')
        {
            printf(attr_name);
            printf("\n");
        }
        else
        {
            printf("NO NAME ATTRIBUTE IN TAG: ");
            printf(tag_content);
            printf("\n");
        }
    }

    free(buffer);

    return 0;
}