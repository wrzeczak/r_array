#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "r_array_template.h"

r_array header_files = RA_STATIC_INIT(RA_STR);

void generation_init();
void register_new_type(const char * typename, const char * associated_type, const char * header_file, const char * comparison_function);
void register_new_pointer_type(const char * typename, const char * associated_type, const char * header_file, const char * comparison_function);
void generate_ra(const char * output_filename);

//------------------------------------------------------------------------------

int main(void) {
    header_files.memory = aa_create(1024);

    register_new_type("RA_DOUBLE", "double", NULL, NULL);
    // the above line creates a type-pair RA_DOUBLE, associated with doubles, which requires no includes and uses == as a comparison
    
    register_new_type("RA_VECTOR2", "Vector2", "<raymath.h>", "Vector2Equals");
    // the above line create a type-pair RA_VECTOR2, associated with Raylib's Vector2, which requires inclusion of <raylib.h> and uses `Vector2Equals` as a comparison

    generate_ra("r_array.h");

    return 0;
}

//------------------------------------------------------------------------------

struct ArrayType {
    const char * typename;
    const char * associated_type;
    const char * comparison_function;
    int header_file_idx;
    bool pointer_type;
};

struct ArrayType * registered_types;
unsigned int num_registered_types = 0;

void generation_init() {
    registered_types = malloc(0);
}

void register_new_type(const char * typename, const char * associated_type, const char * header_file, const char * comparison_function) {
    int idx = -1;
    if(header_file != NULL) {
        int idx_at = ra_member_at(&header_files, header_file);
        if(idx_at >= 0) {
            idx = -1;
        } else {
            ra_append(&header_files, header_file);
            idx = header_files.count - 1;
        }
    }

    struct ArrayType new = { .typename = typename, .associated_type = associated_type, .header_file_idx = idx, .pointer_type = false, .comparison_function = comparison_function };

    num_registered_types++;
    registered_types = realloc(registered_types, num_registered_types * sizeof(struct ArrayType));
    registered_types[num_registered_types - 1] = new;
}

void register_new_pointer_type(const char * typename, const char * associated_type, const char * header_file, const char * comparison_function) {
    int idx = -1;
    if(header_file != NULL) {
        int idx_at = ra_member_at(&header_files, header_file);
        if(idx_at >= 0) {
            idx = -1;
        } else {
            ra_append(&header_files, header_file);
            idx = header_files.count - 1;
        }
    }

    struct ArrayType new = { .typename = typename, .associated_type = associated_type, .header_file_idx = idx, .pointer_type = true, .comparison_function = comparison_function };

    num_registered_types++;
    registered_types = realloc(registered_types, num_registered_types * sizeof(struct ArrayType));
    registered_types[num_registered_types - 1] = new;
}

void generate_ra(const char * output_filename) {
    FILE * output = fopen(output_filename, "w");
    FILE * template = fopen("r_array_template.h", "r");
    char line_buffer[512];

    int step_number = -1;
    bool insert_now = false;

    memset(line_buffer, 0, 512);
    while(fgets(line_buffer, 512, template)) {
        // https://www.geeksforgeeks.org/cpp/strtok-strtok_r-functions-c-examples/

        fprintf(output, "%s", line_buffer);

        char * token;
        char * outer_saveptr = NULL;

        token = strtok_r(line_buffer, " ", &outer_saveptr);

        if(strcmp("//gen", token) == 0) {
            // the format of a generator comment is the following:
            /*
            slash-slash gen (//gen)
            step number, starting with 1 (1)
            comment, surrounded with \" ("Insert new ComponentKind enum values")
            */
           token = strtok_r(NULL, " ", &outer_saveptr);
           // this is the step number
           step_number = strtol(token, NULL, 10);

           printf("Step %02d: ", step_number);

           token = strtok_r(NULL, " ", &outer_saveptr); // get the beginning of the comment
           char comment_buffer[512];
           memset(comment_buffer, 0, 512);
           sprintf(comment_buffer, "%s %s", token, outer_saveptr); // append the rest of the string to the first word of the comment

           comment_buffer[strlen(comment_buffer) - 1] = 0; // trim newline

           printf("%s\n", comment_buffer);

           insert_now = true; // print new lines
        }

        if(insert_now) {
            insert_now = false;

            // these correspond to the manual step numbers in the README
            switch(step_number) {
                case 1: {
                    // including headers
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        // const char * header_file = registered_types[i].header_file;
                        struct ArrayType t = registered_types[i];
                        const char * header_file = (t.header_file_idx >= 0) ? *(const char **) ra_get(&header_files, t.header_file_idx) : NULL;
                        if(header_file == NULL) {
                            printf("\t* No header needed for %s (%s).\n", t.typename, t.associated_type);
                            continue; // no header file needed because this is a primitive
                        }

                        if(header_file[0] == '<') {
                            // use angle brackets
                            fprintf(output, "#include %s // for %s\n", header_file, t.associated_type);
                            fprintf(stdout, "\t* #include %s // for %s\n", header_file, t.associated_type);
                        } else {
                            // use ""
                            fprintf(output, "#include \"%s\" // for %s\n", header_file, t.associated_type);
                            fprintf(stdout, "\t* #include \"%s\" // for %s\n", header_file, t.associated_type);
                        }
                    }

                    break;
                }
                case 2: {
                    // defining types
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        struct ArrayType t = registered_types[i];
                    
                        if(t.pointer_type) {
                            fprintf(output, "ra_type %s = DEFINE_RA_PNT_TYPE(%s); // from %s\n", t.typename, t.associated_type, (t.header_file_idx >= 0) ? *(const char **) ra_get(&header_files, t.header_file_idx) : "stdlib");
                            fprintf(stdout, "\t* ra_type %s = DEFINE_RA_PNT_TYPE(%s); // from %s\n", t.typename, t.associated_type, (t.header_file_idx >= 0) ? *(const char **) ra_get(&header_files, t.header_file_idx) : "stdlib");
                        } else {
                            fprintf(output, "ra_type %s = DEFINE_RA_TYPE(%s); // from %s\n", t.typename, t.associated_type, (t.header_file_idx >= 0) ? *(const char **) ra_get(&header_files, t.header_file_idx) : "stdlib");
                            fprintf(stdout, "\t* ra_type %s = DEFINE_RA_TYPE(%s); // from %s\n", t.typename, t.associated_type, (t.header_file_idx >= 0) ? *(const char **) ra_get(&header_files, t.header_file_idx) : "stdlib");
                        }
                    }

                    break;
                }
                case 3: {
                    // ra_append
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        struct ArrayType t = registered_types[i];
                    
                        if(t.pointer_type) {
                            fprintf(output, "\tRA_APPEND_PNT_TYPE(%s, %s);\n", t.typename, t.associated_type);
                            fprintf(stdout, "\t* RA_APPEND_PNT_TYPE(%s, %s);\n", t.typename, t.associated_type);
                        } else {
                            fprintf(output, "\tRA_APPEND_TYPE(%s, %s);\n", t.typename, t.associated_type);
                            fprintf(stdout, "\t* RA_APPEND_TYPE(%s, %s);\n", t.typename, t.associated_type);
                        }
                    }

                    break;
                }
                case 4: {
                    // ra_member_at
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        struct ArrayType t = registered_types[i];
                    
                        if(t.comparison_function != NULL) {
                            fprintf(output, "\tRA_MEMBER_AT_TYPE_CMP(%s, %s, %s);\n", t.typename, t.associated_type, t.comparison_function);
                            fprintf(stdout, "\t* RA_MEMBER_AT_TYPE_CMP(%s, %s, %s);\n", t.typename, t.associated_type, t.comparison_function);
                        } else {
                            fprintf(output, "\tRA_MEMBER_AT_TYPE(%s, %s);\n", t.typename, t.associated_type);
                            fprintf(stdout, "\t* RA_MEMBER_AT_TYPE(%s, %s);\n", t.typename, t.associated_type);
                        }
                    }

                    break;
                }
                case 5: {
                    // ra_printf
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        struct ArrayType t = registered_types[i];
                        fprintf(output, "\tRA_PRINTF_TYPE(%s, %s);\n", t.typename, t.associated_type);
                        fprintf(stdout, "\t* RA_PRINTF_TYPE(%s, %s);\n", t.typename, t.associated_type);
                    }

                    break;
                }
                default: {
                    printf("ERROR: unrecognized step number. Aborting...\n");
                    fclose(output);
                    fclose(template);
                    exit(2);
                }
            }
        }
    }

    fclose(output);
    fclose(template);
}
