/**
 * @file model_loader.c
 * @brief Model serialization and loading
 */

#include "ai/tensor.h"
#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char name[64];
    nova_tensor_t* tensor;
} model_parameter_t;

typedef struct {
    model_parameter_t* params;
    size_t num_params;
} nova_model_t;

/* Create a new model container */
nova_model_t* nova_model_create(void) {
    nova_model_t* model = nova_alloc(sizeof(nova_model_t));
    if (!model) return NULL;
    model->params = NULL;
    model->num_params = 0;
    return model;
}

/* Add a parameter to the model */
void nova_model_add_param(nova_model_t* model, const char* name, nova_tensor_t* tensor) {
    if (!model || !name || !tensor) return;
    
    model->params = realloc(model->params, (model->num_params + 1) * sizeof(model_parameter_t));
    if (!model->params) return;
    
    strncpy(model->params[model->num_params].name, name, 63);
    model->params[model->num_params].name[63] = '\0';
    model->params[model->num_params].tensor = tensor;
    model->num_params++;
}

/* Get a parameter from the model by name */
nova_tensor_t* nova_model_get_param(nova_model_t* model, const char* name) {
    if (!model || !name) return NULL;
    
    for (size_t i = 0; i < model->num_params; i++) {
        if (strcmp(model->params[i].name, name) == 0) {
            return model->params[i].tensor;
        }
    }
    return NULL;
}

/* Save model to file (simple binary format) */
int nova_model_save(nova_model_t* model, const char* filename) {
    if (!model || !filename) return -1;
    
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    
    // Write header
    uint32_t magic = 0x4E4F5641; // "NOVA"
    fwrite(&magic, sizeof(uint32_t), 1, f);
    fwrite(&model->num_params, sizeof(size_t), 1, f);
    
    // Write each parameter
    for (size_t i = 0; i < model->num_params; i++) {
        model_parameter_t* param = &model->params[i];
        
        // Write name
        fwrite(param->name, sizeof(char), 64, f);
        
        // Write tensor metadata
        fwrite(&param->tensor->ndim, sizeof(size_t), 1, f);
        fwrite(param->tensor->shape, sizeof(size_t), param->tensor->ndim, f);
        fwrite(&param->tensor->dtype, sizeof(int), 1, f);
        
        // Write tensor data
        size_t total_elements = 1;
        for (size_t j = 0; j < param->tensor->ndim; j++) {
            total_elements *= param->tensor->shape[j];
        }
        size_t data_size = total_elements * sizeof(float); // Assume float32
        fwrite(param->tensor->data, 1, data_size, f);
    }
    
    fclose(f);
    return 0;
}

/* Load model from file */
nova_model_t* nova_model_load(const char* filename) {
    if (!filename) return NULL;
    
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    // Read header
    uint32_t magic;
    fread(&magic, sizeof(uint32_t), 1, f);
    if (magic != 0x4E4F5641) {
        fclose(f);
        return NULL;
    }
    
    nova_model_t* model = nova_model_create();
    if (!model) {
        fclose(f);
        return NULL;
    }
    
    size_t num_params;
    fread(&num_params, sizeof(size_t), 1, f);
    
    // Read each parameter
    for (size_t i = 0; i < num_params; i++) {
        char name[64];
        fread(name, sizeof(char), 64, f);
        
        // Read tensor metadata
        size_t ndim;
        fread(&ndim, sizeof(size_t), 1, f);
        
        size_t* shape = nova_alloc(ndim * sizeof(size_t));
        fread(shape, sizeof(size_t), ndim, f);
        
        int dtype;
        fread(&dtype, sizeof(int), 1, f);
        
        // Create tensor
        nova_tensor_t* tensor = nova_tensor_zeros(shape, ndim, dtype);
        nova_free(shape);
        
        if (!tensor) {
            fclose(f);
            return model;
        }
        
        // Read tensor data
        size_t total_elements = 1;
        for (size_t j = 0; j < ndim; j++) {
            total_elements *= tensor->shape[j];
        }
        size_t data_size = total_elements * sizeof(float);
        fread(tensor->data, 1, data_size, f);
        
        nova_model_add_param(model, name, tensor);
    }
    
    fclose(f);
    return model;
}

/* Free model */
void nova_model_free(nova_model_t* model) {
    if (!model) return;
    
    for (size_t i = 0; i < model->num_params; i++) {
        nova_tensor_destroy(model->params[i].tensor);
    }
    
    if (model->params) {
        nova_free(model->params);
    }
    nova_free(model);
}
