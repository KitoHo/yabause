#include "packet.h"
#include "client.h"

void gdb_packet_read(gdb_packet * packet, char * buffer, size_t got)
{
   ssize_t i;
   char c;

   for(i = 0;i < got;i++)
   {
      c = buffer[i];
      switch(packet->state)
      {
         case 0:
            if ('$' == c)
            {
               packet->state = 1;
               packet->buflen = 0;
               packet->bufsum = 0;
            }
            else if (3 == c)
               gdb_client_break(packet->client);
            break;
         case 1:
            if ('#' == c)
            {
               packet->state = 2;
               packet->cslen = 0;
            }
            else
            {
               packet->buffer[packet->buflen++] = c;
               packet->bufsum += c;
            }
            break;
         case 2:
            packet->checksum[packet->cslen++] = c;
            if (2 == packet->cslen)
            {
               u8 checksum;
               sscanf(packet->checksum, "%2x", &checksum);
               packet->buffer[packet->buflen] = '\0';
               if (packet->bufsum == checksum)
                  gdb_client_received(packet->client, packet);
               else
                  gdb_client_error(packet->client);
               packet->state = 0;
            }
            break;
      }
   }
}
