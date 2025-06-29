/**
 * climate.c
 *
 * Performs analysis on climate data provided by the
 * National Oceanic and Atmospheric Administration (NOAA).
 *
 * Input:    Tab-delimited file(s) to analyze.
 * Output:   Summary information about the data.
 *
 * Compile:  run make
 *
 * Example Run:      ./climate data_tn.tdv data_wa.tdv
 *
 *
 * Opening file: data_tn.tdv
 * Opening file: data_wa.tdv
 * States found: TN WA
 * -- State: TN --
 * Number of Records: 17097
 * Average Humidity: 49.4%
 * Average Temperature: 58.3F
 * Max Temperature: 110.4F 
 * Max Temperatuer on: Mon Aug  3 11:00:00 2015
 * Min Temperature: -11.1F
 * Min Temperature on: Fri Feb 20 04:00:00 2015
 * Lightning Strikes: 781
 * Records with Snow Cover: 107
 * Average Cloud Cover: 53.0%
 * -- State: WA --
 * Number of Records: 48357
 * Average Humidity: 61.3%
 * Average Temperature: 52.9F
 * Max Temperature: 125.7F
 * Max Temperature on: Sun Jun 28 17:00:00 2015
 * Min Temperature: -18.7F 
 * Min Temperature on: Wed Dec 30 04:00:00 2015
 * Lightning Strikes: 1190
 * Records with Snow Cover: 1383
 * Average Cloud Cover: 54.5%
 *
 * TDV format:
 *
 * CA» 1428300000000»  9prcjqk3yc80»   93.0»   0.0»100.0»  0.0»95644.0»277.58716
 * CA» 1430308800000»  9prc9sgwvw80»   4.0»0.0»100.0»  0.0»99226.0»282.63037
 * CA» 1428559200000»  9prrremmdqxb»   61.0»   0.0»0.0»0.0»102112.0»   285.07513
 * CA» 1428192000000»  9prkzkcdypgz»   57.0»   0.0»100.0»  0.0»101765.0» 285.21332
 * CA» 1428170400000»  9prdd41tbzeb»   73.0»   0.0»22.0»   0.0»102074.0» 285.10425
 * CA» 1429768800000»  9pr60tz83r2p»   38.0»   0.0»0.0»0.0»101679.0»   283.9342
 * CA» 1428127200000»  9prj93myxe80»   98.0»   0.0»100.0»  0.0»102343.0» 285.75
 * CA» 1428408000000»  9pr49b49zs7z»   93.0»   0.0»100.0»  0.0»100645.0» 285.82413
 *
 * Each field is separated by a tab character \t and ends with a newline \n.
 *
 * Fields:
 *      state code (e.g., CA, TX, etc),
 *      timestamp (time of observation as a UNIX timestamp),
 *      geolocation (geohash string),
 *      humidity (0 - 100%),
 *      snow (1 = snow present, 0 = no snow),
 *      cloud cover (0 - 100%),
 *      lightning strikes (1 = lightning strike, 0 = no lightning),
 *      pressure (Pa),
 *      surface temperature (Kelvin)
 */

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_STATES 50
#define LINE_SIZE 100

struct climate_info {
    char code[3];
    unsigned long num_records;
    long double humidity_sum;
    long double temperature_sum;
    long double max_temp;
    long double min_temp;
    time_t max_temp_timestamp;
    time_t min_temp_timestamp;
    unsigned long lightning_strikes;
    unsigned long snow_cover_records;
    long double cloud_cover_sum;
};

// Function to convert Kelvin to Fahrenheit
double kelvin_to_fahrenheit(double kelvin) {
    return kelvin * 1.8 - 459.67;
}

// Function to create a new state structure
struct climate_info *create_state(char *tokens[]) {
    struct climate_info *state = malloc(sizeof(struct climate_info));
    if (state == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strncpy(state->code, tokens[0], 2);
    state->num_records = 0;
    state->humidity_sum = 0.0;
    state->temperature_sum = 0.0;
    state->max_temp = -DBL_MAX;
    state->min_temp = DBL_MAX;
    state->max_temp_timestamp = 0;
    state->min_temp_timestamp = 0;
    state->lightning_strikes = 0;
    state->snow_cover_records = 0;
    state->cloud_cover_sum = 0.0;

    return state;
}

// Function to update state information
void update_state_info(struct climate_info *states[], struct climate_info *state, int index, char *tokens[]) {
    states[index]->num_records++;
    states[index]->humidity_sum += atof(tokens[3]);
    states[index]->temperature_sum += kelvin_to_fahrenheit(atof(tokens[8]));
    states[index]->cloud_cover_sum += atof(tokens[5]);

    double temp_kelvin = atof(tokens[8]);
    double temp_fahrenheit = kelvin_to_fahrenheit(temp_kelvin);

    if (temp_fahrenheit > states[index]->max_temp) {
        states[index]->max_temp = temp_fahrenheit;
        states[index]->max_temp_timestamp = (time_t)(atoi(tokens[1]) / 1000); // Converted int to time_t
    }

    if (temp_fahrenheit < states[index]->min_temp) {
        states[index]->min_temp = temp_fahrenheit;
        states[index]->min_temp_timestamp = (time_t)(atoi(tokens[1]) / 1000); // Converted int to time_t
    }

    states[index]->lightning_strikes += atoi(tokens[6]);
    states[index]->snow_cover_records += atoi(tokens[4]);
}

// Function to analyze the file and update the states array
void analyze_file(FILE *file, struct climate_info *states[], int num_states) {
    char line[LINE_SIZE];

    while (fgets(line, LINE_SIZE, file) != NULL) {
        char *tokens[9];
        char *token = strtok(line, "\t");
        int tokenIndex = 0;

        while (token != NULL && tokenIndex < 9) {
            tokens[tokenIndex++] = token;
            token = strtok(NULL, "\t");
        }
        
        // If the number of tokens is less than 9, skip this line
        if (tokenIndex < 9) {
            continue;
        }

        // Create a new climate_info struct based on the tokens
        struct climate_info *state = create_state(tokens);

        char *code = state->code;
        int i;

        // Update states array with climate information
        for (i = 0; i < num_states; i++) {
            if (states[i] == NULL) {
                states[i] = state;
                break;
            } else if (strcmp(states[i]->code, code) == 0) {
                update_state_info(states, state, i, tokens);
                break;
            }
        }
    }
}

void print_report(struct climate_info *states[], int num_states) {
    printf("States found:");

    int found_states = 0;
    for (int i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
            found_states++;
            struct climate_info *info = states[i];
            printf(" %s", info->code);
        }
    }

    if (found_states > 0) {
        printf("\n");
    } else {
        printf(" None\n");
    }

    for (int i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
            struct climate_info *info = states[i];
            printf("-- State: %s --\n", info->code);
            printf("Number of Records: %lu\n", info->num_records);
            printf("Average Humidity: %.1Lf%%\n", (info->humidity_sum / info->num_records));
            printf("Average Temperature: %.1Lf°F\n", (info->temperature_sum / info->num_records));
            printf("Max Temperature: %.1Lf°F\n", info->max_temp);
            printf("Max Temperature on: %s", ctime(&info->max_temp_timestamp));
            printf("Min Temperature: %.1Lf°F\n", info->min_temp);
            printf("Min Temperature on: %s", ctime(&info->min_temp_timestamp));
            printf("Lightning Strikes: %lu\n", info->lightning_strikes);
            printf("Records with Snow Cover: %lu\n", info->snow_cover_records);
            printf("Average Cloud Cover: %.1Lf%%\n", (info->cloud_cover_sum / info->num_records));
            printf("---------------------------\n");
        }
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s tdv_file1 tdv_file2 ... tdv_fileN\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct climate_info *states[NUM_STATES] = {NULL};

    for (int i = 1; i < argc; ++i) {
    FILE *file = fopen(argv[i], "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file %s: ", argv[i]);
        perror("");
        // Other actions like skipping this file and continuing, or exiting
        continue; // Move to the next file
    }

    analyze_file(file, states, NUM_STATES);
    fclose(file);
}
    // Print the report containing climate statistics
    print_report(states, NUM_STATES);

    return 0;
}
