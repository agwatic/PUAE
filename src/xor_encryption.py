#!/usr/bin/python2.7

from sys import argv

def xor_encrypt(plainText, key):
    return bytearray([plainText[i] ^ key[i % len(key)]
                      for i in xrange(len(plainText))])

def xor_decrypt(cipherText, key):
    return bytearray([cipherText[i] ^ key[i % len(key)]
                      for i in xrange(len(cipherText))])

def main():
    if len(argv) == 3:
        script_name, filename, keyfilename = argv
    else:
        print "Usage: %s <file to encrypt> <key file>" % script_name
        exit()

    with open(filename, "rb") as file:
        with open(keyfilename, "rb") as key_file:
            bytes = file.read()
            key = key_file.read()
            cipherText = xor_encrypt(bytearray(bytes), bytearray(key))

            newFile = open (filename + ".encrypted", "wb")
            newFile.write(bytearray("AMIGA_RULEZ_XORENC"))
            newFile.write(cipherText)
            newFile.close()

            clearText = xor_decrypt(bytearray(cipherText), bytearray(key))

            if not clearText == bytes:
                print "Error: decrypted text not equal to pre-encryption text!"

if __name__ == '__main__':
    main()
