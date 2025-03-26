/* Remember not to push your JWT to public repo.
    * You can use the following command to ignore changes to this file locally:
    * git update-index --assume-unchanged sta/src/JWT_token.h
    * To start tracking changes again, use:
    * git update-index --no-assume-unchanged sta/src/JWT_token.h
*/
/* Here you define your JWT Authentication token */

// #define AUTH_TOKEN ""

#ifndef AUTH_TOKEN
#error "AUTH_TOKEN is not defined. Please define it appropriately."
#endif