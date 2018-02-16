#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "data_upload.h"
#include "remote_attestation_result.h" //for encrypted data format sp_aes_gcm_data_t
#include "sample_libcrypto.h"

#include "sample_dev_data.h"

#define SAMPLE_SP_IV_SIZE        12

extern sample_aes_gcm_128bit_key_t d_key1;
extern sample_aes_gcm_128bit_key_t d_key2;
extern sample_aes_gcm_128bit_key_t d_key3;
extern sample_aes_gcm_128bit_key_t d_key4;

int data_retrieve(uint8_t dev_id, uint8_t offset, sp_samp_dev_data_t **pp_dev_data){

  sp_samp_dev_data_t *dev_data = NULL;

  dev_data = (sp_samp_dev_data_t *)malloc(sizeof(sp_samp_dev_data_t) + DATA_UPLOAD_SIZE);
  if(NULL == dev_data){
    fprintf(stderr, "\nError, out of memory in [%s].", __FUNCTION__);
    return -1;
  }
  memset(dev_data, 0, sizeof(sp_samp_dev_data_t) + DATA_UPLOAD_SIZE);

  dev_data->size = DATA_UPLOAD_SIZE;

  if(0 == offset){
    memcpy(dev_data->data, &dev_0_data_sample_0[0], sizeof(dev_0_data_sample_0));
  }else if(1 == offset){
    memcpy(dev_data->data, &dev_0_data_sample_1[0], sizeof(dev_0_data_sample_0));
  }else if(2 == offset){
    memcpy(dev_data->data, &dev_0_data_sample_2[0], sizeof(dev_0_data_sample_0));
  }

  *pp_dev_data = dev_data;

  return 0;
}

int sp_upload_data(const char *cloud_storage_url, uint8_t dev_id, uint8_t offset, du_samp_package_header_t **response){

  du_samp_package_header_t *p_resp = NULL;
  sp_aes_gcm_data_t *p_encrypted_data = NULL;
  sp_samp_dev_data_t *p_dev_data = NULL;


  /*
    deliver DATA_UPLOAD_SIZE(8) bytes data of device id to public cloud
  */
  if(0 != data_retrieve(dev_id, offset, &p_dev_data)){
    return -1;
  }

  fprintf(stdout, "\ndata size for uploading: %d \n", p_dev_data->size);
  for(uint8_t i =0; i<p_dev_data->size; i++){
    fprintf(stdout, "data %d: %d\n", i, p_dev_data->data[i]);
  }
  fprintf(stdout, "*********************\n\n");

  uint32_t dev_data_size = (sizeof(sp_samp_dev_data_t) + p_dev_data->size);

  uint8_t aes_gcm_iv[SAMPLE_SP_IV_SIZE] = {0};
  p_encrypted_data = (sp_aes_gcm_data_t *)malloc(sizeof(sp_aes_gcm_data_t) + dev_data_size);
  memset(p_encrypted_data, 0, sizeof(sp_aes_gcm_data_t) + dev_data_size);
  if(!p_encrypted_data){
    fprintf(stderr, "\nError, out of memory in [%s].\n", __FUNCTION__);
    return -1;
  }

  sample_aes_gcm_128bit_key_t dev_key = {0};
  if(0 == dev_id){
    memcpy(&dev_key, &d_key1, sizeof(d_key1));
  }else if(1 == dev_id){
    memcpy(&dev_key, &d_key2, sizeof(d_key1));
  }else if(2 == dev_id){
    memcpy(&dev_key, &d_key3, sizeof(d_key1));
  }else if(3 == dev_id){
    memcpy(&dev_key, &d_key4, sizeof(d_key1));
  }

  int ret = sample_rijndael128GCM_encrypt(&dev_key,
              &p_dev_data->size,
              dev_data_size,
              p_encrypted_data->payload,
              &aes_gcm_iv[0],
              SAMPLE_SP_IV_SIZE,
              NULL,
              0,
              &p_encrypted_data->payload_tag);
  if(SAMPLE_SUCCESS != ret){
    fprintf(stderr, "\nError, data encryption in [%s].\n", __FUNCTION__);
    return -1;
  }

  p_encrypted_data->payload_size = dev_data_size;

  p_resp = (du_samp_package_header_t *)malloc(sizeof(du_samp_package_header_t) + sizeof(sp_aes_gcm_data_t) + dev_data_size);
  if(!p_resp){
    fprintf(stderr, "\nError, out of memory in [%s].", __FUNCTION__);
    return -1;
  }
  memset(p_resp, 0, sizeof(du_samp_package_header_t) + sizeof(sp_aes_gcm_data_t) + dev_data_size);

  if(0 == dev_id){
    p_resp->type = TYPE_DEVICE_0;
  }else if(1 == dev_id){
    p_resp->type = TYPE_DEVICE_1;
  }else if(2 == dev_id){
    p_resp->type = TYPE_DEVICE_2;
  }else if(3 == dev_id){
    p_resp->type = TYPE_DEVICE_3;
  }

  p_resp->size = sizeof(sp_aes_gcm_data_t) + dev_data_size;

  memcpy(p_resp->body, p_encrypted_data, sizeof(sp_aes_gcm_data_t) + dev_data_size);

  *response = p_resp;

  return 0;

}
