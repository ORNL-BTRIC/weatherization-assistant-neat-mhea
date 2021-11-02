/***************************************************************************
 * MODULE:	size.h            CREATED:	April 22, 1999 , 2009
 *
 * AUTHOR:	g9a ORNL
 *          Mark Fishbaugher  Fishbaugher and Associates  1999
 *
 * MDESC:	External function prototypes for Manual J sizing functions
 *          in the size.c module
 ****************************************************************************/
#ifndef _SIZE_H
#define _SIZE_H

void sizing_heating(int rt);
void sizing_cooling(int rt);
void report_manual_j(FILE *filetmp);

#endif /* _SIZE_H */
