/**
 * @file zenith_autocal_run.c
 * @brief CLI tool for running Nova auto-calibration
 */

#include "zenith_autocal.h"
#include <stdio.h>
#include <string.h>

void print_usage(const char *prog)
{
    printf("Nova Auto-Calibration Tool\n");
    printf("\nUsage: %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  --full         Run comprehensive calibration (all benchmarks)\n");
    printf("  --quick        Run quick calibration (essential benchmarks only)\n");
    printf("  --load <file>  Load configuration from file\n");
    printf("  --save <file>  Save configuration to file\n");
    printf("  --reset        Reset to default values\n");
    printf("  --help         Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --full                  # Full calibration\n", prog);
    printf("  %s --quick                 # Quick calibration\n", prog);
    printf("  %s --load config.json      # Load existing config\n", prog);
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        // Default: run comprehensive calibration
        zenith_autocal_run_comprehensive();
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--full") == 0) {
            zenith_autocal_run_comprehensive();
            return 0;
        }
        else if (strcmp(argv[i], "--quick") == 0) {
            zenith_autocal_run_quick();
            return 0;
        }
        else if (strcmp(argv[i], "--load") == 0) {
            if (i + 1 < argc) {
                if (zenith_autocal_load(argv[i + 1])) {
                    printf("Configuration loaded successfully\n");
                } else {
                    printf("Failed to load configuration\n");
                    return 1;
                }
                i++;
            } else {
                printf("Error: --load requires a filename\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--save") == 0) {
            if (i + 1 < argc) {
                if (zenith_autocal_save(argv[i + 1])) {
                    printf("Configuration saved successfully\n");
                } else {
                    printf("Failed to save configuration\n");
                    return 1;
                }
                i++;
            } else {
                printf("Error: --save requires a filename\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--reset") == 0) {
            zenith_autocal_reset_defaults();
            zenith_autocal_save("nova_autocal_config.json");
            return 0;
        }
        else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    return 0;
}
