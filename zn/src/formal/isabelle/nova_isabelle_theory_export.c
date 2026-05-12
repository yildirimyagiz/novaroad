#include <stdio.h>
#include <stdlib.h>

void nova_isabelle_export_data_type(FILE *f, const char *name,
                                      const char **constructors, int count) {
  fprintf(f, "datatype %s = ", name);
  for (int i = 0; i < count; i++) {
    fprintf(f, "%s%s", constructors[i], i == count - 1 ? "" : " | ");
  }
  fprintf(f, "\n");
}

int nova_isabelle_export_theory(const char *spec_name, const char *content,
                                  const char *path) {
  printf("[Gödel/Isabelle] Exporting theory '%s' to %s...\n", spec_name, path);
  FILE *f = fopen(path, "w");
  if (!f)
    return -1;
  fprintf(f, "theory %s\nimports Main\nbegin\n\n%s\n\nend\n", spec_name,
          content);
  fclose(f);
  return 0;
}
