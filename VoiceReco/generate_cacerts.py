import os

def is_newer_than(file1, file2):
    """
    Checks if file1 is newer than file2 based on modification time.

    Args:
        file1 (str): Path to the first file.
        file2 (str): Path to the second file.

    Returns:
        bool: True if file1 is newer than file2, False otherwise.
              Returns False if either file does not exist.
    """
    if not os.path.exists(file1) or not os.path.exists(file2):
        return False

    return os.path.getmtime(file1) > os.path.getmtime(file2)


# crt_path = os.path.realpath('/etc/ssl/certs/ca-certificates.crt')
# if is_newer_than(crt_path, 'src/CACert.cpp'):
#     with open(crt_path, 'r') as f:
#         content = f.read()
#
#     with open('src/CACert.cpp', 'w') as f:
#         f.write(rf'''#include "CACert.h"
#
# const char caCerts[] = R"CACERT({content})CACERT";''')

crt_path = os.path.realpath('lib/baidu-com-chain.pem')
if is_newer_than(crt_path, 'src/CACert.cpp'):
    with open(crt_path, 'r') as f:
        content = f.read()

    with open('src/CACert.cpp', 'w') as f:
        f.write(rf'''#include "CACert.h"

const char certBaiduCom[] = R"CACERT({content})CACERT";''')
