/* makeweb.c - let a user create a web subdirectory
**
** Copyright � 1995 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

/* This is intended to be installed setgid to a group that has
** write access to the system web directory.  It allows any user
** to create a subdirectory there.  It also makes a symbolic link
** in the user's home directory pointing at the new web subdir.
*/


#include "../config.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../checkedc_utils.h"


#define LINK "public_html"

#pragma CHECKED_SCOPE on

static char *argv0 : itype(_Nt_array_ptr<char>) = ((void *)0);


_Checked static void
check_room( int size, int len )
    {
    if ( len > size )
	{
	(void) fprintf( stderr, "%s: internal error, out of room\n", argv0 );
	exit( 1 );
	}
    }


_Checked static void
end_with_slash(char *str : itype(_Nt_array_ptr<char>) count(4999))
    {
    size_t len = strlen(str);
    if ( str[len - 1] != '/' )
	(void) xstrbcat( str, "/", 4999 );
    }


_Checked static void
check_dir(char *dirname : itype(_Nt_array_ptr<char>) count(4999), uid_t uid, gid_t gid)
    {
    struct stat sb;

    /* Check the directory. */
    if ( stat( dirname, &sb ) < 0 )
	{
	if ( errno != ENOENT )
	    {
	    perror( dirname );
	    exit( 1 );
	    }
	/* Doesn't exist.  Try to make it. */
	if ( mkdir( dirname, 0755 ) < 0 )
	    {
	    if ( errno == ENOENT )
		(void) printf( "\
Some part of the path %s does not exist.\n\
This is probably a configuration error.\n", dirname );
	    else
		perror( dirname );
	    exit( 1 );
	    }
	(void) printf( "Created web directory %s\n", dirname );
	/* Try to change the group of the new dir to the user's group. */
	_Unchecked { (void) chown( (char*) dirname, -1, gid ); }
	}
    else
	{
	/* The directory already exists.  Well, check that it is in
	** fact a directory.
	*/
	if ( ! S_ISDIR( sb.st_mode ) )
	    {
	    (void) printf(
		"%s already exists but is not a directory!\n", dirname );
	    exit( 1 );
	    }
	if ( sb.st_uid != uid )
	    {
	    (void) printf(
		"%s already exists but you don't own it!\n", dirname );
	    exit( 1 );
	    }
	(void) printf( "Web directory %s already existed.\n", dirname );
	}
    }


_Checked int
main(int argc, char **argv : itype(_Array_ptr<_Nt_array_ptr<char>>) count(argc))
    {
    _Nt_array_ptr<char> webdir : byte_count(14) = ((void *)0);
    _Nt_array_ptr<char> prefix : byte_count(0) = ((void *)0);
    _Ptr<struct passwd> pwd = ((void *)0);
    _Nt_array_ptr<char> username = 0;
    _Nt_array_ptr<char> homedir = 0;
    char dirname _Nt_checked[5000];
    char linkname _Nt_checked[5000];
    char linkbuf _Nt_checked[5000];
    struct stat sb;

    argv0 = argv[0];
    if ( argc != 1 )
	{
	(void) fprintf( stderr, "usage:  %s\n", argv0 );
	exit( 1 );
	}

    _Unchecked { pwd = _Assume_bounds_cast<_Ptr<struct passwd>>(getpwuid( getuid() )); }
    if ( pwd == 0 )
	{
	(void) fprintf( stderr, "%s: can't find your username\n", argv0 );
	exit( 1 );
	}
    _Unchecked { username = _Assume_bounds_cast<_Nt_array_ptr<char>>(pwd->pw_name, count(0)); }
    _Unchecked { homedir = _Assume_bounds_cast<_Nt_array_ptr<char>>(pwd->pw_dir, count(0)); }

#ifdef TILDE_MAP_2

    /* All we have to do for the TILDE_MAP_2 case is make sure there's
    ** a public_html subdirectory.
    */
    check_room(
	sizeof(dirname), strlen( homedir ) + strlen( TILDE_MAP_2 ) + 2 );
    (void) xstrbcpy( dirname, homedir, sizeof(dirname) - 1 );
    end_with_slash( dirname, sizeof(dirname) - 1 );
    (void) xstrbcat( dirname, TILDE_MAP_2, sizeof(dirname) - 1 );

    check_dir( dirname, pwd->pw_uid, pwd->pw_gid );

#else /* TILDE_MAP_2 */

    /* Gather the pieces. */
    webdir = WEBDIR;
#ifdef TILDE_MAP_1
    prefix = TILDE_MAP_1;
#else /* TILDE_MAP_1 */
    prefix = "";
#endif /* TILDE_MAP_1 */

    /* Assemble the directory name.  Be paranoid cause we're sgid. */
    check_room(
	sizeof(dirname),
	strlen( webdir ) + strlen( prefix ) + strlen( username ) + 3 );
    (void) xstrbcpy( dirname, webdir, sizeof(dirname) - 1 );
    end_with_slash(dirname);
    if ( strlen( prefix ) != 0 )
	{
	(void) xstrbcat( dirname, prefix, sizeof(dirname) - 1 );
	end_with_slash(dirname);
	}
    (void) xstrbcat( dirname, username, sizeof(dirname) - 1 );

    /* Assemble the link name. */
    check_room( sizeof(linkname), strlen( homedir ) + strlen( LINK ) + 2 );
    (void) xstrbcpy( linkname, homedir, sizeof(linkname) - 1 );
    end_with_slash(linkname);
    (void) xstrbcat( linkname, LINK, sizeof(linkname) - 1 );

    check_dir( dirname, pwd->pw_uid, pwd->pw_gid );

    /* Check the symlink. */
    try_link_again: ;
    if ( lstat( linkname, &sb ) < 0 )
	{
	if ( errno != ENOENT )
	    {
	    perror( linkname );
	    exit( 1 );
	    }
	/* Doesn't exist.  Try to make it. */
        int res = 0;
        _Unchecked { res = symlink((char*) dirname, (char*) linkname); }
	if ( res < 0 )
	    {
	    if ( errno == ENOENT )
		(void) printf( "\
Some part of the path %s does not exist.\n\
This is probably a configuration error.\n", linkname );
	    else
		perror( linkname );
	    exit( 1 );
	    }
	(void) printf( "Created symbolic link %s\n", linkname );
	}
    else
	{
	/* The link already exists.  Well, check that it is in
	** fact a link.
	*/
	if ( ! S_ISLNK( sb.st_mode ) )
	    {
	    (void) printf( "\
%s already exists but is not a\n\
symbolic link!  Perhaps you have a real web subdirectory in your\n\
home dir from a previous web server configuration?  You may have\n\
to rename it, run %s again, and then copy in the old\n\
contents.\n", linkname, argv0 );
	    exit( 1 );
	    }
	/* Check the existing link's contents. */
	if ( readlink( linkname, linkbuf, sizeof(linkbuf) - 1 ) < 0 )
	    {
	    perror( linkname );
	    exit( 1 );
	    }
	if ( strcmp( dirname, linkbuf ) == 0 )
	    (void) printf( "Symbolic link %s already existed.\n", linkname );
	else
	    {
	    (void) printf( "\
Symbolic link %s already existed\n\
but it points to the wrong place!  Attempting to remove and\n\
recreate it.\n", linkname );
            int res = 0;
            _Unchecked { unlink( (char*) linkname); }
	    if ( res < 0 )
		{
		perror( linkname );
		exit( 1 );
		}
	    goto try_link_again;
	    }
	}
#endif /* TILDE_MAP_2 */

    exit( 0 );
    }
