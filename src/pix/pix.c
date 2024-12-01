#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include <sys/stat.h>
#include <libexif/exif-data.h>
#include <time.h>
#include <errno.h>
#include <strings.h>

#ifndef PIX_VERSION
#define PIX_VERSION "unknown"
#endif

#define MAX_PATH_LEN 1024
#define MAX_DATE_LEN 256

typedef struct {
    char format[MAX_DATE_LEN];
    int preset;
    int simulate;
    int recursive;
    int show_version;
    int show_help;
} Options;

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] FILE | DIRECTORY\n", prog_name);
    printf("Rename photos and videos based on EXIF capture timestamp.\n\n");
    printf("Options:\n");
    printf("  -p, --preset <NUMBER>      Use predefined format preset (default is 0):\n");
    printf("                               0: %s\n", "%Y%m%d%H%M%S%f");
    printf("                               1: %s\n", "%Y%m%d_%H%M%S_%f");
    printf("                               2: %s\n", "%Y%m%d-%H%M%S-%f");
    printf("                               3: %s\n", "%Y-%m-%d_%H-%M-%S_%f");
    printf("                               4: %s\n", "%Y_%m_%d-%H_%M_%S-%f");
    printf("  -f, --format <FORMAT>      Use custom format (strftime-compatible; overrides preset)\n");
    printf("  -s, --simulate             Perform a dry run (do not rename files)\n");
    printf("  -r, --recursive            Recursively process directories\n");
    printf("  -v, --version              Show program version\n");
    printf("  -h, --help                 Show this help message\n");
    printf("\n");
    printf("A target FILE or DIRECTORY must be specified.\n");
    printf("If a FILE is specified, only that file will be processed.\n");
    printf("If a DIRECTORY is specified, files in that directory will be processed.\n");
    printf("Use '.' to specify the current directory.\n");
}


int is_image_or_video(const char *filename) {
    const char *extensions[] = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tif", ".tiff",
        ".mp4", ".mov", ".avi", ".mkv", ".heic", ".heif", ".webp"
    };
    size_t num_extensions = sizeof(extensions) / sizeof(extensions[0]);
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    for (size_t i = 0; i < num_extensions; ++i) {
        if (strcasecmp(ext, extensions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void process_file(const char *filepath, const Options *options);

void traverse_directory(const char *dirpath, const Options *options, int recursive) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "Error opening directory '%s': %s\n", dirpath, strerror(errno));
        return;
    }

    struct dirent *entry;
    char path[MAX_PATH_LEN];

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, MAX_PATH_LEN, "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(path, &st) != 0) {
            fprintf(stderr, "Error getting status of '%s': %s\n", path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (recursive) {
                traverse_directory(path, options, recursive);
            }
        } else if (S_ISREG(st.st_mode)) {
            if (is_image_or_video(entry->d_name)) {
                process_file(path, options);
            }
        }
    }

    closedir(dir);
}

void sanitize_filename(char *filename) {
    // Replace any illegal characters with '_'
    for (char *p = filename; *p; ++p) {
        if (*p == '/' || *p == '\\' || *p == ':' || *p == '*' || *p == '?' ||
            *p == '"' || *p == '<' || *p == '>' || *p == '|') {
            *p = '_';
        }
    }
}

void process_file(const char *filepath, const Options *options) {
    ExifData *ed = exif_data_new_from_file(filepath);
    if (!ed) {
        fprintf(stderr, "Warning: No EXIF data found in '%s'\n", filepath);
        return;
    }

    ExifEntry *entry = exif_content_get_entry(ed->ifd[EXIF_IFD_EXIF], EXIF_TAG_DATE_TIME_ORIGINAL);
    if (!entry) {
        fprintf(stderr, "Warning: No DateTimeOriginal EXIF tag found in '%s'\n", filepath);
        exif_data_unref(ed);
        return;
    }

    char date_str[MAX_DATE_LEN];
    exif_entry_get_value(entry, date_str, sizeof(date_str));
    if (date_str[0] == '\0') {
        fprintf(stderr, "Warning: Empty DateTimeOriginal in '%s'\n", filepath);
        exif_data_unref(ed);
        return;
    }

    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    if (strptime(date_str, "%Y:%m:%d %H:%M:%S", &tm) == NULL) {
        fprintf(stderr, "Warning: Failed to parse date '%s' in '%s'\n", date_str, filepath);
        exif_data_unref(ed);
        return;
    }

    // Get sub-second time if available
    char subsec[7] = "000000"; // Ensure subsec is 6 characters long
    ExifEntry *subsec_entry = exif_content_get_entry(ed->ifd[EXIF_IFD_EXIF], EXIF_TAG_SUB_SEC_TIME_ORIGINAL);
    if (subsec_entry) {
        char subsec_value[20];
        exif_entry_get_value(subsec_entry, subsec_value, sizeof(subsec_value));
        if (subsec_value[0] != '\0') {
            // Ensure subsec is always 6 characters
            size_t len = strlen(subsec_value);
            if (len >=6) {
                // Take the first six characters
                strncpy(subsec, subsec_value, 6);
                subsec[6] = '\0';
            } else {
                // Pad with zeros on the right
                strcpy(subsec, subsec_value);
                for (size_t i = len; i < 6; ++i) {
                    subsec[i] = '0';
                }
                subsec[6] = '\0';
            }
        }
    }

    exif_data_unref(ed);

    char new_name[MAX_PATH_LEN];
    char format_str[MAX_DATE_LEN];
    strcpy(format_str, options->format);

    // Replace %f with microseconds
    char *f_ptr = strstr(format_str, "%f");
    if (f_ptr) {
        char before_f[MAX_DATE_LEN];
        char after_f[MAX_DATE_LEN];
        size_t before_len = f_ptr - format_str;
        strncpy(before_f, format_str, before_len);
        before_f[before_len] = '\0';
        strcpy(after_f, f_ptr + 2); // Skip over "%f"

        char time_str[MAX_DATE_LEN];
        strftime(time_str, sizeof(time_str), before_f, &tm);

        char after_time_str[MAX_DATE_LEN];
        strftime(after_time_str, sizeof(after_time_str), after_f, &tm);

        snprintf(new_name, sizeof(new_name), "%s%s%s", time_str, subsec, after_time_str);
    } else {
        strftime(new_name, sizeof(new_name), format_str, &tm);
    }

    sanitize_filename(new_name);

    // Get the file extension
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        ext = ""; // No extension
    }

    // Prepare new path
    char new_path[MAX_PATH_LEN];
    char dirpath[MAX_PATH_LEN];
    strcpy(dirpath, filepath);
    char *last_slash = strrchr(dirpath, '/');
    if (last_slash) {
        *last_slash = '\0';
    } else {
        strcpy(dirpath, ".");
    }
    snprintf(new_path, sizeof(new_path), "%s/%s%s", dirpath, new_name, ext);

    // If the current file is already correctly named, skip renaming
    if (strcmp(filepath, new_path) == 0) {
        // File is already correctly named
        return;
    }

    // Check if new_path exists and is not the current file
    if (access(new_path, F_OK) == 0 && strcmp(filepath, new_path) != 0) {
        // Handle filename collision
        int counter = 1;
        char temp_new_path[MAX_PATH_LEN];
        do {
            snprintf(temp_new_path, sizeof(temp_new_path), "%s/%s_%d%s", dirpath, new_name, counter++, ext);
        } while (access(temp_new_path, F_OK) == 0 && strcmp(filepath, temp_new_path) != 0);
        strcpy(new_path, temp_new_path);
    }

    if (options->simulate) {
        printf("Simulating '%s' -> '%s'\n", filepath, new_path);
    } else {
        if (rename(filepath, new_path) != 0) {
            fprintf(stderr, "Error renaming '%s' to '%s': %s\n", filepath, new_path, strerror(errno));
        } else {
            printf("Renamed '%s' -> '%s'\n", filepath, new_path);
        }
    }
}

int main(int argc, char *argv[]) {
    Options options = {0};
    options.preset = 0; // Default preset
    options.simulate = 0;
    options.recursive = 0;

    static struct option long_options[] = {
        {"preset", required_argument, 0, 'p'},
        {"format", required_argument, 0, 'f'},
        {"simulate", no_argument, 0, 's'},
        {"recursive", no_argument, 0, 'r'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "p:f:srvh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                options.preset = atoi(optarg);
                break;
            case 'f':
                if (strlen(optarg) >= MAX_DATE_LEN) {
                    fprintf(stderr, "Error: Format string too long.\n");
                    exit(EXIT_FAILURE);
                }
                strcpy(options.format, optarg);
                break;
            case 's':
                options.simulate = 1;
                break;
            case 'r':
                options.recursive = 1;
                break;
            case 'v':
                printf("%s\n", PIX_VERSION);
                exit(EXIT_SUCCESS);
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Set format based on preset if custom format not provided
    if (options.format[0] == '\0') {
        switch (options.preset) {
            case 0:
                strcpy(options.format, "%Y%m%d%H%M%S%f");
                break;
            case 1:
                strcpy(options.format, "%Y%m%d_%H%M%S_%f");
                break;
            case 2:
                strcpy(options.format, "%Y%m%d-%H%M%S-%f");
                break;
            case 3:
                strcpy(options.format, "%Y-%m-%d_%H-%M-%S_%f");
                break;
            case 4:
                strcpy(options.format, "%Y_%m_%d-%H_%M_%S-%f");
                break;
            default:
                fprintf(stderr, "Error: Invalid preset number %d.\n", options.preset);
                exit(EXIT_FAILURE);
        }
    }

    int args_left = argc - optind;
    if (args_left == 0) {
        fprintf(stderr, "Error: No target file or directory specified.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (args_left > 1) {
        fprintf(stderr, "Error: Too many arguments provided.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (args_left == 1) {
        char *path = argv[optind];
        struct stat st;
        if (stat(path, &st) != 0) {
            fprintf(stderr, "Error: Cannot access '%s': %s\n", path, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (S_ISREG(st.st_mode)) {
            if (options.recursive) {
                fprintf(stderr, "Error: Recursive option '-r' is not compatible with specifying a file.\n");
                exit(EXIT_FAILURE);
            }
            if (is_image_or_video(path)) {
                process_file(path, &options);
            } else {
                fprintf(stderr, "Warning: '%s' is not a supported image or video file.\n", path);
            }
        } else if (S_ISDIR(st.st_mode)) {
            traverse_directory(path, &options, options.recursive);
        } else {
            fprintf(stderr, "Error: '%s' is neither a file nor a directory.\n", path);
            exit(EXIT_FAILURE);
        }
    } else {
        // No path specified, use current directory
        if (options.recursive) {
            traverse_directory(".", &options, 1);
        } else {
            traverse_directory(".", &options, 0);
        }
    }

    return 0;
}
