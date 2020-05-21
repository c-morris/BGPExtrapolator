#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>

#include <iostream>
#include <string.h>

#define MESSEGE_SIZE 24
#define HMAC_KEY_LENGTH 32

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Please specify the ammount of announcements");
        return 0;
    }

    unsigned int num_announcements = atoi(argv[1]);

    unsigned char messege[MESSEGE_SIZE];
    unsigned char hash[SHA256_DIGEST_LENGTH];

    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

    clock_t start, end;

    double ecdsa_verification_time_total = 0;

    //ECDSA p-256 with SHA-256
    for(unsigned int t = 0; t < num_announcements; t++) {
        //Alter messege
        RAND_bytes(messege, MESSEGE_SIZE);

        //Re-hash
        SHA256(messege, MESSEGE_SIZE, hash);

        //Get new key
        EC_KEY_generate_key(key);

        //Sign it
        ECDSA_SIG* signature = ECDSA_do_sign(hash, SHA256_DIGEST_LENGTH, key);

        start = clock();

        //Verify
        ECDSA_do_verify(hash, SHA256_DIGEST_LENGTH, signature, key);

        end = clock();

        ecdsa_verification_time_total += (double) (end - start) / CLOCKS_PER_SEC;

        //cleanup
        ECDSA_SIG_free(signature);
    }

    printf("Time spent verifying ECDSA p-256 with SHA-256 hashing: %f (seconds)\n", ecdsa_verification_time_total);
    printf("Average time to verify ECDSA p-256 with SHA-256 hashing: %f (milliseconds)\n\n", ((ecdsa_verification_time_total / (double) num_announcements) * 1000.0));
    
    //cleanup
    EC_KEY_free(key);

    double hmac_verification_time_total = 0;

    clock_t start2, end2;

    unsigned char hkey[HMAC_KEY_LENGTH];
    unsigned char fake_original_auth_code[SHA256_DIGEST_LENGTH];
    for(unsigned int i = 0; i < num_announcements; i++) {
        //New Key
        RAND_bytes(hkey, HMAC_KEY_LENGTH);

        //Alter messege
        RAND_bytes(messege, MESSEGE_SIZE);

        start = clock();

        unsigned char* auth_code = HMAC(EVP_sha256(), hkey, HMAC_KEY_LENGTH, messege, MESSEGE_SIZE, NULL, NULL);

        end = clock();

        //Since we need to compare the code to the "original", copy this into a new variable (off the timer) to then compare against on the timer
        memcpy(fake_original_auth_code, auth_code, SHA256_DIGEST_LENGTH);

        start2 = clock();

        if(memcmp(fake_original_auth_code, auth_code, SHA256_DIGEST_LENGTH) != 0) {
            printf("Unequal\n");
        }

        end2 = clock();

        hmac_verification_time_total += (double) ((end - start) + (end2 - start2)) / CLOCKS_PER_SEC;
    }

    printf("Total time spent verifying HMAC with SHA-256 hashing: %f (seconds)\n", hmac_verification_time_total);
    printf("Average time to verify HMAC with SHA-256 hashing: %f (milliseconds)\n", ((hmac_verification_time_total / (double) num_announcements) * 1000.0));

    return 0;
}
