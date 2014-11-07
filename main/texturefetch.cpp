#include "texturefetch.hpp"
#include "raytracing.h"
#include <iostream>
#include <assert.h>

/*
 *  Pour la documentation de ces fonctions, se rapporter à la page "Textures" de la documentation
 */
/*
 * @brief getTexel Get a single texel from a texture.
 * This function access the texture at coordinates (row, column) and fetch the value of the corresponding texel (pixel from a texture)
 * @param pixels    The image to access, organized as a linear array of texel arranged by row
 * @param width     Width of the image
 * @param height    Height of the image
 * @param depth     Depth of the image (number of component by texel)
 * @param row       Row coordinate of the requested texel
 * @param column    Column coordinate of the requested texel
 * @return          The value of the texel
 * @todo            Transfom the 2D coordinates in a 1D index and get the corresponding texel value
 */
Color getTexel(unsigned char *pixels, int width, int height, int depth, int column, int row){
    // TODO : multiply by 1.0/255 the color

    int index = (column * depth) + (row * (width*depth));
    float red = ((float)pixels[index+0])/255;
    float green = ((float)pixels[index+1])/255;
    float blue = ((float)pixels[index+2])/255;
    Color color(red,green,blue);

    return color;
}

void setTexel(unsigned char *pixels, int width, int height, int depth, int column, int row, Color color){
    // TODO: use sse ?
    int index = (column * depth) + (row * (width*depth));
    pixels[index + 0] = color[0]*255;
    pixels[index + 1] = color[1]*255;
    pixels[index + 2] = color[2]*255;
}

/*
 * @brief interpolateTexture Get a texel linearly interpolated from its neighbors
 * @param pixels    The image to access, organized as a linear array of texel arranged by row
 * @param width     Width of the image
 * @param height    Height of the image
 * @param depth     Depth of the image (number of component by texel)
 * @param s         The column coordinate of the requested texel as a floating point
 * @param t         The row coordinate of the requested texel as a floating point
 * @return          The value of the interpolated texel
 * @todo            Devoir 3, 2014 : From the floating point coordinates, compute the integer part and the fractional part. The integer part is the used to acces the 4 texels implied in the interpolation (One texel and its 3 up and right neighors), the fractional part is used to linearly interpolate from neighbors.
 */
Color interpolateTexture(unsigned char *pixels, int width, int height, int depth, float s, float t){
    // TODO: use sse
    int xFloor = std::floor(s);
    int yFloor = std::floor(t);
    Color texelDownL = getTexel(pixels, width, height, depth, xFloor, yFloor);
    Color texelDownR = getTexel(pixels, width, height, depth, xFloor+ 1, yFloor);
    Color texelTopL = getTexel(pixels, width, height, depth, xFloor, yFloor + 1);
    Color texelTopR = getTexel(pixels, width, height, depth, xFloor+ 1, yFloor+ 1);

    float distanceX = s - xFloor;
    float distanceY = t - yFloor;
    float distanceX1 = 1.0f - distanceX;
    float distanceY1 = 1.0f - distanceY;

    float w1 = distanceX1 * distanceY1 ;
    float w2 = distanceX  * distanceY1 ;
    float w3 = distanceX1 * distanceY  ;
    float w4 = distanceX  * distanceY  ;

    float red = texelDownL[0] * w1 + texelDownR[0] * w2 + texelTopL[0] * w3 + texelTopR[0] * w4 ;
    float green = texelDownL[1] * w1 + texelDownR[1] * w2 + texelTopL[1] * w3 + texelTopR[1] * w4 ;
    float blue = texelDownL[2] * w1 + texelDownR[2] * w2 + texelTopL[2] * w3 + texelTopR[2] * w4 ;

    return Color(red,green,blue);
}


/*
 * @brief integrateTexture Get a texel by computing the mean of the color on a neighborood of size (deltas x deltat)
 * @param pixels    The image to access, organized as a linear array of texel arranged by row
 * @param width     Width of the image
 * @param height    Height of the image
 * @param depth     Depth of the image (number of component by texel)
 * @param s         The column coordinate of the requested texel as a floating point
 * @param t         The row coordinate of the requested texel as a floating point
 * @param deltas    The size, in the column dimension, of the neighborood
 * @param deltat    The size, in the row dimension, of the neighborood
 * @return
 * @todo
 */
Color integrateTexture(unsigned char *pixels, int width, int height, int depth, float s, float t, int deltas, int deltat){

    int x = std::floor(s);
    int y = std::floor(t);
    int maxX = x + deltas;
    int maxY = y+deltat;
    Color c = init_color (0.f, 0.f, 0.f);
    Color temp;

    for(x= std::floor(s) ; x < maxX ; x++)
    {
        for(y= std::floor(t); y < maxY ; y++)
        {

            temp = getTexel(pixels, width, height, depth, x,y);
            c = add_color_color(c,temp);
        }
    }

    float nb = 1.f/(deltas * deltat);
    return (c*nb);
}

/*
 * @brief prefilterTexture Compute an array of images with geometrically decreasing resolution from the original image.
 * @param imagearray The array of images to compute. element at index 0 in this array is the full resolution image and must not be modified
 * @param width     Width of the full resolution image
 * @param height    Height of the full resolution image
 * @param depth     Depth of the image (number of component by texel)
 * @param nblevels  Number of level to compute : nblevels = log2(min(width, height))
 * @return          if the array may be filled, return true, else return false
 */
bool prefilterTexture(unsigned char **imagearray, int width, int height, int depth, int nblevels){
    for(int i = 1; i < nblevels ; i++)
    {
        // bitwise more efficient than divide, same as width/2 & height/2 here
        width = width >> 1;
        height = height >> 1;
        imagearray[i] = (unsigned char *)calloc(width*height*depth,sizeof(unsigned char));
        for(int x = 0; x < width ; x++)
        {
            for(int y = 0; y < height ; y++)
            {
                // we integrate the i-1 image.
                // width = half_width * 2 and height = half_height * 2
                Color color = integrateTexture(imagearray[i-1],width * 2 , height * 2, depth,(float)2*x,(float)2*y,2,2 );
                setTexel(imagearray[i],width,height,depth,x,y,color);
            }
        }
    }
        return true;
}

