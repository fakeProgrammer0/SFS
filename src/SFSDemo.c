#include "SFS.h"

void testSFSOpenCloseAPI();

void main()
{
    runSFS();

    // while(1)
    //     testSFSOpenCloseAPI();
}

void testSFSOpenCloseAPI()
{
    printf("Test SFS_open && SFS_close API.\n");
    // const int 1024 = 1024;
    char username[1024] = {};
    char password[1024] = {};
    char filename[1024] = {};
    char mode[1024] = {};

    acceptALineFromCMD(username, "username: ");
    acceptAPasword(password, "password: ");
    acceptALineFromCMD(filename, "filename: ");
    acceptALineFromCMD(mode, "mode: ");

    FILE *fp = SFS_open(username, password, filename, mode);
    if (fp == NULL)
    {
        printf("File [%s] can't be opened!\n", filename);
    }
    else
    {
        printf("File [%s] is opened!\n", filename);

        char ch = '\n';
        do
        {
            ch = getchar();

            if (ch != EOF)
                fputc(ch, fp);
            else
                break;
        } while (1);

        // const int N = 100;
        // for (int i = 0; i < N; i++)
        // {
        //     fprintf(fp, "%d ", i);
        //     if ((i + 1) % 10 == 0)
        //         fprintf(fp, "\n");
        // }

        int x = SFS_close(username, password, filename, mode, fp);
        if (x == 0)
        {
            printf("File closes successfully!\n");
        }

        printf("Done!\n");
    }
    acceptEnter("Press [Enter] to continue...");
}
