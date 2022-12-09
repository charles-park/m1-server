#
# efuse control test app. charles.park 2022/12/09
#
import sys
import os
import asyncio

from efuse import ODROID_M1

if __name__ == "__main__":
    args = sys.argv

    args_cnt = len(args)
    board_name = 'None'
    uuid = 'None'

    if args_cnt < 3:
        print ('usage : python3 efuse_ctl.py [-w|-d|-e] [-w:uuid|-e:offset] [-w:offset]')
        print ('      e.g) python3 efuse_ctl.py -w dcbaa404-91bd-4a63-b5f1-001e06428067 0')
        print ('           python3 efuse_ctl.py -e 0')
        quit()

    # odroid-m1 efuse control
    board = ODROID_M1()

    if args_cnt == 4:
        print(args[2], int(args[3]))

    if args[1] == '-w':
        success = board.provision(args[2], int(args[3]))
    elif args[1] == '-d':
        success = board.dump()
    elif args[1] == '-e':
        success = board.clear(args[2], int(args[3]))
    else :
        quit()

    if not success:
        print('ERROR')
    else:
        print('OK')
