#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "lodepng.h"


#define VERSION "1.1"



#define LIMIT_NR_PIXELS (8000L*10000L)
#define LIMIT_PNG_SIZE 20000000

#define NARROW_LINE_COLOR 0x00333333


#define MAX_ERODE_COUNT 40

#define MAX_DISTANCE (8000L+10000L)

#define ENABLE_ERODE_IN
#define ERODE_IN_COUNT 2
#define ERODE_OUT_COUNT 3


#ifdef RUN_LOCAL

#endif //RUN_LOCAL

#define ENABLE_SIMPLE_SMALL_PATCH_REMOVER
#define SMALL_PATCHES_WARNINGS_RED
#define SMALL_PATCHES_WARNINGS_COLOR 0xff

#ifdef ENABLE_SIMPLE_SMALL_PATCH_REMOVER
#define SMALL_PATCH_LIMIT ((1L*width*height)/40000) // 50 // 40000
#define SMALL_PATCH_REMOVE_LIMIT ((1L*width*height)/10000)

#define ULTRA_TINY_PATCH_LIMIT 4

#endif // ENABLE_SIMPLE_SMALL_PATCH_REMOVER

int nrareas = 1;


unsigned width, height;
unsigned int* area_index_per_pixel = NULL;
uint16_t *distances = NULL;




#define MAX_QUEUE_ELEMS LIMIT_NR_PIXELS
static int *queue = NULL;
static int nrqueElems=0;

#define IS_WHITE(_img,_c) ((_img)[(_c)] >= 252)


static void hsv_to_rgb(double h, double s, double v, double& r, double& g, double& b) 
{
  int i;
  double f, p, q, t;

  s /= 255.0;
  v /= 255.0;
	
  if(s < 0.01)
  {
    // Achromatic (grey)
    r = g = b = v;
    r*=255;
    g*=255;
    b*=255;
    r = (int)(r+0.5);
    g = (int)(g+0.5);
    b = (int)(b+0.5);
    return;
  }

  h = ((int)h)%360;
	
  h /= 60.0; // sector 0 to 5
  i = (int)(h);
  f = h - i; 
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));

  switch(i) 
  {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    default: // case 5:
      r = v;
      g = p;
      b = q;
  }

  r*=255;
  g*=255;
  b*=255;
  r = (int)(r+0.5);
  g = (int)(g+0.5);
  b = (int)(b+0.5);
  return;
}



static void FloodFill(unsigned char *image_gray, unsigned int* colors, long coord, int color)
{
  if (queue == NULL)
  {
    queue = (int*)malloc(MAX_QUEUE_ELEMS*sizeof(int));
  }

  int w = width;
  
  nrqueElems = 0;
  queue[nrqueElems++] = coord;

  while (nrqueElems>0)
  {
    nrqueElems--;
    coord = queue[nrqueElems];
    if (colors[coord] != 0)
      continue;
    colors[coord] = color;
    int x = coord%(w);
    int y = coord/(w);
    if (x>0)
    {
      coord = (x-1)+y*w;
      if (IS_WHITE(image_gray,coord) && colors[coord] == 0)
        queue[nrqueElems++] = coord;
    }    
    if (y>0)
    {
      coord = x+(y-1)*w;
      if (IS_WHITE(image_gray,coord) && colors[coord] == 0)
        queue[nrqueElems++] = coord;
    }    

    if (x<width-1)
    {
      coord = (x+1)+y*w;
      if (IS_WHITE(image_gray,coord) && colors[coord] == 0)
       queue[nrqueElems++] = coord;
    }    
    if (y<height-1)
    {
      coord = x+(y+1)*w;
      if (IS_WHITE(image_gray,coord) && colors[coord] == 0)
        queue[nrqueElems++] = coord;
    }
  }
}




static void FloodFillForce(unsigned int* colors, int coord, int color)
{
  int prev_color = colors[coord];
  int w = width;
  
  nrqueElems = 0;
  queue[nrqueElems++] = coord;

  while (nrqueElems>0)
  {
    nrqueElems--;
    coord = queue[nrqueElems];
    if (colors[coord] != prev_color)
      continue;
    colors[coord] = color;
    int x = coord%(w);
    int y = coord/(w);
    if (x>0)
    {
      coord = (x-1)+y*w;
      if (colors[coord] == prev_color)
        queue[nrqueElems++] = coord;
    }    
    if (y>0)
    {
      coord = x+(y-1)*w;
      if (colors[coord] == prev_color)
        queue[nrqueElems++] = coord;
    }    

    if (x<width-1)
    {
      coord = (x+1)+y*w;
      if (colors[coord] == prev_color)
       queue[nrqueElems++] = coord;
    }    
    if (y<height-1)
    {
      coord = x+(y+1)*w;
      if (colors[coord] == prev_color)
        queue[nrqueElems++] = coord;
    }
  }
}





static long FloodFillCount(unsigned int* colors, int coord)
{
  int color = colors[coord];
  long count = 0;
  int w = width;
  
  nrqueElems = 0;
  queue[nrqueElems++] = coord;

  while (nrqueElems>0)
  {
    nrqueElems--;
    coord = queue[nrqueElems];
    if (colors[coord] == 0)
      continue;
    colors[coord] = 0;
    count++;
    int x = coord%(w);
    int y = coord/(w);
    if (x>0)
    {
      coord = (x-1)+y*w;
      if (colors[coord] == color)
        queue[nrqueElems++] = coord;
    }    
    if (y>0)
    {
      coord = x+(y-1)*w;
      if (colors[coord] == color)
        queue[nrqueElems++] = coord;
    }    

    if (x<width-1)
    {
      coord = (x+1)+y*w;
      if (colors[coord] == color)
       queue[nrqueElems++] = coord;
    }    
    if (y<height-1)
    {
      coord = x+(y+1)*w;
      if (colors[coord] == color)
        queue[nrqueElems++] = coord;
    }
  }
  return count;
}








static unsigned int FloodFillCountSingleNeighborColor(unsigned int* colors, int coord)
{
  unsigned int singleNeighborColor = 0;
  int hasMultiple = 0;

  int color = colors[coord];
  long count = 0;
  int w = width;
  
  nrqueElems = 0;
  queue[nrqueElems++] = coord;

//#define CHECK_NEIGHBOR if (colors[coord] != 0) { singleNeighborColor = colors[coord]; fprintf(flogging, "%d\n", singleNeighborColor); }



#define CHECK_NEIGHBOR \
  { \
    if (colors[coord] != 0 && !hasMultiple) \
    { \
      if (singleNeighborColor == 0) \
      { \
        singleNeighborColor = colors[coord]; \
      } \
      else if (singleNeighborColor != colors[coord]) \
      { \
        hasMultiple=1; \
      }\
    }\
  }

  while (nrqueElems>0)
  {
    nrqueElems--;
    coord = queue[nrqueElems];
    if (colors[coord] == 0)
      continue;
    colors[coord] = 0;
    count++;

    int x = coord%(w);
    int y = coord/(w);
    if (x>0)
    {
      coord = (x-1)+y*w;
      if (colors[coord] == color)
      {
        queue[nrqueElems++] = coord;
      }
      else
      {
        CHECK_NEIGHBOR
      }
    }    
    if (y>0)
    {
      coord = x+(y-1)*w;
      if (colors[coord] == color)
      {
        queue[nrqueElems++] = coord;
      }
      else
      {
        CHECK_NEIGHBOR
      }
    }    

    if (x<width-1)
    {
      coord = (x+1)+y*w;
      if (colors[coord] == color)
      {
       queue[nrqueElems++] = coord;
      }
      else
      {
        CHECK_NEIGHBOR
      }
    }    
    if (y<height-1)
    {
      coord = x+(y+1)*w;
      if (colors[coord] == color)
      {
        queue[nrqueElems++] = coord;
      }
      else
      {
        CHECK_NEIGHBOR
      }
    }
  }
  if (count>=SMALL_PATCH_REMOVE_LIMIT)
  {
    hasMultiple=1;
  }

  return (hasMultiple ? 0 : singleNeighborColor);
}







static double rnd_rng(int x)
{
  x = (x<<13) ^ x;
  return ( 1.0 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

double rnd_rng_0_to_1(int x)
{
  return (1+rnd_rng(x))/2;
}

static unsigned int RandomColor(int x)
{
/*
  double h = 360*rnd_rng_0_to_1(3*x);
  double s = 8+36*rnd_rng_0_to_1(3*x+1);
  double v = 200+48*rnd_rng_0_to_1(3*x+2);
*/

  double h = 360*rnd_rng_0_to_1(3*x);
  double s = 8+72*rnd_rng_0_to_1(3*x+1);
  double v = 184+64*rnd_rng_0_to_1(3*x+2);


  double r,g,b;
  hsv_to_rgb(h, s, v, r, g, b);   

  unsigned char ucr=(unsigned char)r;
  unsigned char ucg=(unsigned char)g;
  unsigned char ucb=(unsigned char)b;

  int ir = ucr;
  int ig = ucg;
  int ib = ucb;

 return (ir<<16) | (ig<<8) | ib;
}

#define MARK_ERROR "\n########################\n"

static void Usage(const char* exename)
{
  fprintf(stderr,"Usage: %s [input.png] [output.png]\n",exename);
  fprintf(stderr,"\tIf output.png is not provided, the result is saved to \"flats.png\" by default, and if \"input.png\" is not provided, the input line art is read from \"lines.png\" by default.\n\n");
  fprintf(stderr,"[PRESS ENTER]\n");
  char temp[256];
  fgets(temp,255,stdin);
  exit(-10);
}


int main(int argc, char** argv)
{

  fprintf(stderr,"Flatton, offline version " VERSION "\nCopyright Ayal Pinkus 2018\n");

  const char* inputName = "lines.png";
  const char* outputName = "flats.png";

  if (argc>1)
  {
    inputName = argv[1];
  }
  if (argc>2)
  {
    outputName = argv[2];
  }

  fprintf(stderr,"Flatting \"%s\" and storing result in \"%s\"\n", inputName, outputName);

  long contentLength=0;
  unsigned char* content = NULL;

  {
    FILE*f=fopen(inputName,"rb");
    if (!f)
    {
      fprintf(stderr,MARK_ERROR "ERROR: could not open file %s for reading." MARK_ERROR,inputName);
      Usage(argv[0]);
    }
    fseek(f,0,SEEK_END);
    contentLength=ftell(f);
    fseek(f,0,SEEK_SET);
    content = (unsigned char*)malloc(contentLength+1);
    fread(content, 1, contentLength, f);
    content[contentLength] = 0;
    fclose(f);
  }

//printf("CONTENT_LENGTH = %s\n", getenv("CONTENT_LENGTH"));
  
  const unsigned char pngHeader[] = 
  {
    137,80,78,71,13,10,26,10,0
  };


  const unsigned char* headerPtr = (const unsigned char*)strstr((const char*)content, (const char*)pngHeader);
  if (headerPtr == NULL)
  {
    fprintf(stderr,MARK_ERROR "ERROR: The input file is not a png image. Only png is currently supported." MARK_ERROR);
    Usage(argv[0]);
  }
  else
  {
//    printf("Image found.<p>\n");
//    printf("Offset = %ld<p>\n",(headerPtr-content));




    int pnglen = contentLength - (headerPtr-content);

    if (pnglen > LIMIT_PNG_SIZE)
    {
      fprintf(stderr,MARK_ERROR "ERROR: Image is %d bytes, limit is %d bytes (you can increase this limit in flatton.cpp)." MARK_ERROR,pnglen, LIMIT_PNG_SIZE);
      Usage(argv[0]);
    }
    else
    {
      unsigned error;
      unsigned char* image=NULL;
      unsigned char* image_gray=NULL;
      unsigned char* png_out = NULL;
      size_t pngsize_out = 0;
      
      error = lodepng_decode32(&image, &width, &height, headerPtr, pnglen);
      if(error)
      {
        printf("PNG load error %u: %s\n", error, lodepng_error_text(error));
        Usage(argv[0]);
      }
      else
      {
        unsigned long nrpixels = width;
        nrpixels*= height;
        if (nrpixels> LIMIT_NR_PIXELS)
	{
          fprintf(stderr,MARK_ERROR "ERROR: Image size is %dx%d, or %ld pixels. Limit is %ld. (you can increase this limit in flatton.cpp)." MARK_ERROR, width, height, nrpixels, LIMIT_NR_PIXELS);
          Usage(argv[0]);
	}
	else
        {
          int j;
          unsigned long x, y;
	  area_index_per_pixel = (unsigned int*)malloc(sizeof(unsigned int)*width*height);
          image_gray=(unsigned char*)malloc(sizeof(unsigned char)*width*height);;
          memset(area_index_per_pixel,0,sizeof(unsigned int)*width*height);

          long i;
          for (i=width*height-1;i>=0;i--)
          {
	    if (image[4*i+3] < 52)
	    {
              image_gray[i] = 255;
	    }
	    else
	    {
              int gray = image[4*i];
	      gray += image[4*i+1];
	      gray += image[4*i+2];
              image_gray[i] = ((gray >= 204+204+204) ? 255 : 0);
            }
          }


          if (image)
          {
            free(image);
	    image = NULL;
          }


#ifdef ENABLE_SIMPLE_SMALL_PATCH_REMOVER
          unsigned int *scrap_colors = (unsigned int*)malloc(sizeof(unsigned int)*width*height);
#endif // ENABLE_SIMPLE_SMALL_PATCH_REMOVER


          // Erode black lines in gray image so as to close narrow gaps
#ifdef ENABLE_ERODE_IN
          unsigned char* image_gray_eroded=(unsigned char*)malloc(sizeof(unsigned char)*width*height);;
          unsigned char* image_gray_scrap=(unsigned char*)malloc(sizeof(unsigned char)*width*height);;
          memcpy(image_gray_eroded, image_gray, sizeof(unsigned char)*width*height);
	  for (j=0;j<ERODE_IN_COUNT;j++)
	  {
            memcpy(image_gray_scrap, image_gray_eroded, sizeof(unsigned char)*width*height);
            for (y=0;y<height;y++)
	    {
	      for (x=0;x<width;x++)
	      {
	        if (image_gray_scrap[x+y*width])
		{
		  if (x>0) if (!image_gray_scrap[x-1+y    *width]) {image_gray_eroded[x+y*width] = 0; continue;}
		  if (y>0) if (!image_gray_scrap[x  +(y-1)*width]) {image_gray_eroded[x+y*width] = 0; continue;}
		  if (x<width-1) if (!image_gray_scrap[x+1+y    *width]) {image_gray_eroded[x+y*width] = 0; continue;}
		  if (y<height-1) if (!image_gray_scrap[x +(y+1)*width]) {image_gray_eroded[x+y*width] = 0; continue;}
	        }
	      }
	    }
	  }
          for (i=width*height-1;i>=0;i--)
          {
            if (IS_WHITE(image_gray_eroded,i))
            {
              FloodFill(image_gray_eroded, area_index_per_pixel, i, nrareas++);
            }
          }

          // Erode colors back up to black lines
	  for (j=0;j<ERODE_OUT_COUNT;j++)
	  {
            memcpy(scrap_colors, area_index_per_pixel, sizeof(unsigned int)*width*height);
            for (y=0;y<height;y++)
	    {
	      for (x=0;x<width;x++)
	      {
	        if (IS_WHITE(image_gray,i) && !scrap_colors[x+y*width])
		{
		  if (x>0) if (scrap_colors[x-1+y    *width]) {area_index_per_pixel[x+y*width      ] = scrap_colors[x-1+y    *width]; continue;}
		  if (y>0) if (scrap_colors[x  +(y-1)*width]) {area_index_per_pixel[x+y*width      ] = scrap_colors[x  +(y-1)*width]; continue;}
		  if (x<width-1)  if (scrap_colors[x+1+y    *width]) {area_index_per_pixel[x+y*width] = scrap_colors[x+1+y    *width]; continue;}
		  if (y<height-1) if (scrap_colors[x  +(y+1)*width]) {area_index_per_pixel[x+y*width] = scrap_colors[x  +(y+1)*width]; continue;}
	        }
              }
	    }
	  }

          // Finished
	  free(image_gray_eroded);
	  free(image_gray_scrap);
#else // ENABLE_ERODE_IN

          for (i=width*height-1;i>=0;i--)
          {
            if (IS_WHITE(image_gray,i))
            {
              FloodFill(image_gray, area_index_per_pixel, i, nrareas++);
            }
          }

#endif // ENABLE_ERODE_IN









/*
  Very small patches are made black at this point. This because the flood fill algorithms look 
  at the four directions, and so single pixels can be cornered and grow into annoying full-fledged 
  fake color areas. Here they are removed so the areas around it can erode into it.
*/

#ifdef ENABLE_SIMPLE_SMALL_PATCH_REMOVER
          memcpy(scrap_colors, area_index_per_pixel, sizeof(unsigned int)*width*height);
          for (i=width*height-1;i>=0;i--)
          {
            if (area_index_per_pixel[i] != 0 && scrap_colors[i] != 0)
            {
              long count = FloodFillCount(scrap_colors, i);
              if (count > 0 && count<ULTRA_TINY_PATCH_LIMIT)
              {
                FloodFillForce(area_index_per_pixel, i, 0);
              }
            }
          }

#endif // ENABLE_SIMPLE_SMALL_PATCH_REMOVER










/////////////////////////////
//
// BEGIN ERODE
//
/////////////////////////////

          unsigned int *new_area_index_per_pixel = (unsigned int*)malloc(sizeof(unsigned int)*width*height);

          int count=0;
          bool changed = true;
          while (changed)
          {
            count++;

            if (count > MAX_ERODE_COUNT)
            {
             break;
            }
    
            for (i=0;i<width*height;i++)
            {
	      new_area_index_per_pixel[i] = area_index_per_pixel[i];
	    }
            changed = false;
            for (y=1;y<height-1;y++)
            {
              for (x=1;x<width-1;x++)
              {
                if (area_index_per_pixel[x+y*width] != 0)
                  continue;

                int neighs[9];
                int neighcount[9];
                int nrneighs = 0;

                int ix,iy, in;
                for (ix=x-1;ix<=x+1;ix++)
                {
                  for (iy=y-1;iy<=y+1;iy++)
                  {
	            int coord=ix+iy*width;
          	    if (area_index_per_pixel[coord] != 0)
	            {
          	      for (in=0;in<nrneighs;in++)
	              {
	                if (neighs[in] == area_index_per_pixel[coord])
        	        {
	                  neighcount[in]++;
  	                  break;
        	        }
	              }
        	      if (in == nrneighs)
	              {
   	                neighs[nrneighs] = area_index_per_pixel[coord];
        	        neighcount[nrneighs] = 1;
	                nrneighs++;
        	      }
	            }
                  }
                }
                if (nrneighs > 0)
                {
                  int max=neighcount[0];
	          int imax=0;
          	  for (in=1;in<nrneighs;in++)
	          {
        	    if (neighcount[in]>max)
  	            {
        	      max = neighcount[in];
	              imax = in;
        	    }
	          }
                  new_area_index_per_pixel[x+y*width] = neighs[imax];
                  changed = true;
                }
              }
            }
            unsigned int *swap = new_area_index_per_pixel;
            new_area_index_per_pixel = area_index_per_pixel;
            area_index_per_pixel = swap;
          }

          free(new_area_index_per_pixel);
          new_area_index_per_pixel = NULL;


/////////////////////////////
//
// END ERODE
//
/////////////////////////////




/*
  Patches that are really small are turned into red here. After that, red patches that are still 
  too small need to be dealt with. The code checks if there is one neighbor (the area fully enclosed 
  by one other color), or more than one. If it has only one neighbor, it takes on that color, otherwise
  it stays red. If the red patches (that can now be combinations of other areas that were previously 
  too small) are now bigger than the threshold, they remain red also. 
 */

#ifdef ENABLE_SIMPLE_SMALL_PATCH_REMOVER
          memcpy(scrap_colors, area_index_per_pixel, sizeof(unsigned int)*width*height);
#ifdef SMALL_PATCHES_WARNINGS_RED
int smallPatchIndex = nrareas;
#else
int smallPatchIndex = 0;
#endif // SMALL_PATCHES_WARNINGS_RED

          for (i=width*height-1;i>=0;i--)
          {
            if (area_index_per_pixel[i] != smallPatchIndex && scrap_colors[i] != 0)
            {
              long count = FloodFillCount(scrap_colors, i);
              if (count > 0 && count<SMALL_PATCH_LIMIT)
              {
                FloodFillForce(area_index_per_pixel, i, smallPatchIndex);
              }
            }
          }
	  

#ifdef SMALL_PATCHES_WARNINGS_RED
          nrareas++;
#endif // SMALL_PATCHES_WARNINGS_RED

#endif // ENABLE_SIMPLE_SMALL_PATCH_REMOVER




#ifdef ENABLE_SIMPLE_SMALL_PATCH_REMOVER

#ifdef SMALL_PATCHES_WARNINGS_RED

          memcpy(scrap_colors, area_index_per_pixel, sizeof(unsigned int)*width*height);
          for (i=width*height-1;i>=0;i--)
          {
            if (area_index_per_pixel[i] == smallPatchIndex && scrap_colors[i] != 0)
            {
              unsigned int singleNeighborColor = FloodFillCountSingleNeighborColor(scrap_colors, i);
              if (singleNeighborColor)
              {
                FloodFillForce(area_index_per_pixel, i, singleNeighborColor);
              }
            }
	  }
#endif // SMALL_PATCHES_WARNINGS_RED
	  
	  free(scrap_colors);
	  scrap_colors=NULL;
#endif // ENABLE_SIMPLE_SMALL_PATCH_REMOVER



#define RECORDED_NR_BYTES (256L*256L*256L/8)
         unsigned char* recorded = (unsigned char*)malloc(RECORDED_NR_BYTES);
         memset(recorded, 0, RECORDED_NR_BYTES);
#define RECORD_COLOR(_c) { recorded[(_c)>>3] |= (1<<((_c)&7)); }
#define COLOR_WAS_ALREADY_RECORDED(_c) (recorded[(_c)>>3] & (1<<((_c)&7)))

          unsigned int *colormap = (unsigned int *)malloc(nrareas*sizeof(unsigned int));
          colormap[0] = NARROW_LINE_COLOR;
          RECORD_COLOR(NARROW_LINE_COLOR)

          int count_limit=0;
          for (i=1;i<nrareas;i++)
          {
	    unsigned int newcolor = RandomColor(i);
            if (count_limit < 3 && !COLOR_WAS_ALREADY_RECORDED(newcolor))
	    {
              count_limit=0;
              colormap[i] = newcolor;
              RECORD_COLOR(NARROW_LINE_COLOR)
	    }
	    else
	    {
	      count_limit++;
	      i--;
	    }
          }
          free(recorded);
          recorded = NULL;

          unsigned char* image_out = (unsigned char*)malloc(width * height * 3);




#ifdef SMALL_PATCHES_WARNINGS_RED
          colormap[nrareas-1] = SMALL_PATCHES_WARNINGS_COLOR;
#endif // SMALL_PATCHES_WARNINGS_RED

          for (i=width*height-1;i>=0;i--)
          {
            unsigned int c = colormap[area_index_per_pixel[i]];
            image_out[3*(i)+0] = c      &0xff;
            image_out[3*(i)+1] = (c>>8) &0xff;
            image_out[3*(i)+2] = (c>>16)&0xff;
          }
          free(colormap);
	  colormap = NULL;

          free(area_index_per_pixel);
	  area_index_per_pixel=NULL;

          unsigned error = lodepng_encode24(&png_out, &pngsize_out, image_out, width, height);

          // if there's an error, display it
          if(error)
          {
            printf("error %u: %s\n", error, lodepng_error_text(error));
	    return 0;
          }
          free(image_out);
	  image_out = NULL;
        }
      }

      if (image_gray)
      {
        free(image_gray);
	image_gray = NULL;
      }

      if (queue)
      {
        free(queue);
	queue = NULL;
      }

      {
	FILE*f=fopen(outputName,"wb");
	if (!f)
	{
	  fprintf(stderr,MARK_ERROR "ERROR: could not open file %s for writing." MARK_ERROR,outputName);
          Usage(argv[0]);
	}
	fwrite(png_out, 1, pngsize_out, f);
	fclose(f);
      }
    }
  }
  
  free(content);
  return 0;
}



