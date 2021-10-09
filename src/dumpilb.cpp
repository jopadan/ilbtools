#include <stdio.h>
#include <stdlib.h>

// First, a tiny subroutine to read 4 byte integers,
// required to make it CPU independent. Actually, you
// could simply use fread(integer,1,4,f) on any Win machine
int read_int (FILE *f)
{
	unsigned int a,b,c,d;

	a = fgetc(f);
	b = fgetc(f);
	c = fgetc(f);
	d = fgetc(f);

	return (signed int)(a | (b<<8) | (c<<16) | (d<<24));
}

// The classic invocation for a command line program:
int main (int argc, char **argv)
{
// We need a FILE object to read from.
	FILE *file;
// ..as well as a few variables
	int h,i, type, InfoByte, size, numPalette, isComposite;

// Let's first check if the command line was properly formed
	if (argc != 2)
	{
		printf ("You forgot this program needs one single input.\n"
				"Supply a filename (and only one)\n");
		exit(EXIT_FAILURE);
	}

// It'd better be a file that can be read
	file = fopen (argv[1], "rb");
	if (file == NULL)
	{
		printf ("Unable to read your input %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

// Check the magic value
	h = read_int (file);
	if (h != 0x424C4904)
	{
		fclose (file);
		printf ("Your input %s is not an ILB file\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	printf ("Magic ID: %08Xh\n", h);

// Read & discard unknown value
	h = read_int (file);
	printf ("Unknown: %08Xh (%d)\n", h, h);

// Read version number
	h = read_int (file);
	if (h == 0x40400000)
			printf ("Version: %08Xh (3.0)\n", h);
	else
		if (h == 0x40800000)
			printf ("Version: %08Xh (4.0)\n", h);
		else
		{
			fclose (file);
			printf ("Unknown version %08Xh\n", h);
			exit(EXIT_FAILURE);
		}

// Read header length
	h = read_int (file);
	printf ("Hdr Length: %08Xh (%d)\n", h, h);

// v3.0 headers are done. If that last value
// was 24, it's a v4.0 header
	if (h != 16 && h != 24)
	{
			fclose (file);
			printf ("Unknown header length!\n");
			exit(EXIT_FAILURE);
	}
// Display extra info
	if (h == 16)
	{
		// Unknown (palette?)
		h = read_int (file);
		printf ("Unknown: %08Xh (%d)\n", h, h);
	}
	if (h == 24)
	{
		h = read_int (file);
		printf ("Img Directory: %08Xh (%d) bytes\n", h, h);
		h = read_int (file);
		printf ("File length: %08Xh (%d) bytes\n", h, h);

		// We save the number of palettes, so
		// if needed, we can check if they're there.
		numPalette = read_int (file);
		printf ("Palettes: %d\n", numPalette);
		// Read the palettes
		while (numPalette-- > 0)
		{
			i = read_int (file);
			printf ("  Palette id: %08Xh\n", i);
			if (i != 0x88801B18)
			{
				fclose (file);
				printf ("Unknown palette id!\n");
				exit(EXIT_FAILURE);
			}
			// Skip actual palette data (no need to show'em)
			for (i=0; i<256; i++)
			{
				read_int (file);
			}
		}
	}

	isComposite = 0;

	// This is an endless loop.
	while (1)
	{

// Skip the id for composites!
// A simple check for not zero:
		if (!isComposite)
		{
			// This images' identifier
			printf ("================\n");
			h = read_int (file);
			printf ("Id: %08Xh (#%d)\n", h, h);
		} else
		{
			printf ("----------------\n");
		}

		// We're done if this is an end code.
		// If so, break out of the endless loop.
		if (h == 0xffffffff)
		{
			// This should not be happening:
			if (isComposite)
			{
				printf ("Error: End of file in a composite!\n");
			}
			break;
		}

		// Either isComposite or an image type
		// We need the type a few times, so
		// save it in another variable.
		type = read_int (file);
		if (type == 256)
		{
			// Yup. Set the flag, read actual type.
			printf ("Composite image: %08Xh\n", type);
			isComposite = 1;
			type = read_int (file);
		}
		printf ("Image type: %d = ", type);
		// Check for empty image or end of composite
		if (type == 0)
		{
			// Yup. Reset the flag and loop to the beginning.
			if (isComposite)
				printf ("End composite.\n");
			else
				printf ("Empty.\n");
			isComposite = 0;
			// Skip the end value first
			h = read_int (file);
			printf ("Empty flag: %08Xh\n\n", h);
			continue;
		}

	// Test type; stop on unknown ones
		switch (type)
		{
			case  1:
				printf ("Picture08\n");
				fclose (file);
				exit(EXIT_FAILURE);
			case  2: printf ("RLESprite08\n"); break;
			case 16: printf ("Picture16\n"); break;
			case 17: printf ("RLESprite16\n"); break;
			case 18: printf ("TransparentRLESprite16\n"); break;
			case 19:
				printf ("BitMask\n");
				fclose (file);
				exit(EXIT_FAILURE);
			case 20:
				printf ("Shadow\n");
				fclose (file);
				exit(EXIT_FAILURE);
			case 21:
				printf ("TransparentPicture16\n");
				fclose (file);
				exit(EXIT_FAILURE);
			case 22: printf ("Sprite16\n"); break;
			default:
				printf ("unknown\n");
				fclose (file);
				exit(EXIT_FAILURE);
		}

		// A single important byte
		InfoByte = fgetc (file);
		printf ("InfoByte: %d\n", InfoByte);

	// The length of the name
		h = read_int (file);
		printf ("Name length: %d bytes\n", h);
	// Basic sanity checking...
		if (h < 0 || h > 100)
		{
				printf ("Rather unrealistic, I'm afraid.\n");
				fclose (file);
				exit(EXIT_FAILURE);
		}
		printf ("Name: ");
	// A simple reading loop
	// You could check if the characters are
	// all valid for a file name.
		while (h-- > 0)
		{
			i = fgetc (file);
			printf ("%c", i);
		}
		printf ("\n");
	// Basic image parameters
		h = read_int (file);
		printf ("Image width: %d\n", h);
		h = read_int (file);
		printf ("Image height: %d\n", h);
		h = read_int (file);
		printf ("X offset: %d\n", h);
		h = read_int (file);
		printf ("Y offset: %d\n", h);

	// Only of interest for composites:
		h = read_int (file);
		printf ("Subid: %d\n", h);

	// Our first unknown byte!
		h = fgetc (file);
		printf ("UnknownA: %02Xh (%d)\n", h, h);

	// The image data size; save it for now
		size = read_int (file);
		printf ("Data size: %d bytes\n", size);

	// If InfoByte = 1 (and the version is 3.0)
	// there is no data offset
		if (InfoByte != 1)
		{
			h = read_int (file);
			printf ("Data offset: %d\n", h);
		}

	// The offset width and height
		h = read_int (file);
		printf ("Offset width: %d\n", h);
		h = read_int (file);
		printf ("Offset height: %d\n", h);

	// A switch on each image type
	// No need to check again for unknown types.
		switch (type)
		{
			case  2:  // RLESprite08
				// Unknown byte and 2 ints
				h = fgetc (file);
				printf ("UnknownB: %02Xh (%d)\n", h, h);
				h = read_int (file);
				printf ("UnknownC: %08Xh (%d)\n", h, h);
				h = read_int (file);
				printf ("UnknownD: %08Xh (%d)\n", h, h);
				// Palette number to use
				h = read_int (file);
				printf ("Palette #: %d\n", h);
				// Palette sanity check!
				// I don't consider this fatal--for now...
				if (h < 0 || h >= numPalette)
				{
					printf ("Palette number out of range!\n");
				}
				h = read_int (file);
				printf ("Clipwidth: %d\n", h);
				h = read_int (file);
				printf ("Clipheight: %d\n", h);
				h = read_int (file);
				printf ("Clip X offset: %d\n", h);
				h = read_int (file);
				printf ("Clip Y offset: %d\n", h);
				h = read_int (file);
				printf ("Transparency index: %d\n", h);
				// A final unknown integer
				h = read_int (file);
				printf ("UnknownE: %08Xh (%d)\n", h, h);
				break;

			case 16:  // Picture16
				// Blend info only if InfoByte is 3
				if (InfoByte == 3)
				{
					h = read_int (file);
					printf ("ShowMode: %08Xh", h);
					// Parse this into its components
					switch (h & 0x000000ff)
					{
						case 0: printf (" = smOpaque"); break;
						case 1: printf (" = smTransparent"); break;
						case 2: printf (" = smBlended, ");
							// Show extra blend info
							// Include unknown ones just for good measure
							switch ((h >> 8) & 0x000000ff)
							{
								case 0: printf ("bmUser"); break;
								case 1: printf ("bmAlpha"); break;
								case 2: printf ("bmBrighten"); break;
								case 3: printf ("bmIntensity"); break;
								case 4: printf ("bmShadow"); break;
								case 5: printf ("bmLinearAlpha"); break;
								default: printf ("Unknown");
							}
							break;
						default: printf (" -- Unknown");
					}
					printf ("\n");
					// This one is always there, although
					// only used with smBlended mode
					h = read_int (file);
					printf ("BlendValue: %d\n", h);
				}
				h = read_int (file);
				printf ("PixelFormat: %08Xh\n", h);
				break;

			// The next two contain the same data:
			case 17:  // RLESprite16
			case 18:  // TransparentRLESprite16
				// Blend info only if InfoByte is 3
				if (InfoByte == 3)
				{
					h = read_int (file);
					printf ("ShowMode: %08Xh", h);
					// Parse this into its components
					switch (h & 0x000000ff)
					{
						case 0: printf (" = smOpaque"); break;
						case 1: printf (" = smTransparent"); break;
						case 2: printf (" = smBlended, ");
							// Show extra blend info
							// Include unknown ones just for good measure
							switch ((h >> 8) & 0x000000ff)
							{
								case 0: printf ("bmUser"); break;
								case 1: printf ("bmAlpha"); break;
								case 2: printf ("bmBrighten"); break;
								case 3: printf ("bmIntensity"); break;
								case 4: printf ("bmShadow"); break;
								case 5: printf ("bmLinearAlpha"); break;
								default: printf ("Unknown");
							}
							break;
						default: printf (" -- Unknown");
					}
					printf ("\n");
					// This one is always there, although
					// only used with smBlended mode
					h = read_int (file);
					printf ("BlendValue: %d\n", h);
				}
				h = read_int (file);
				printf ("PixelFormat: %08Xh\n", h);
				h = read_int (file);
				printf ("Clipwidth: %d\n", h);
				h = read_int (file);
				printf ("Clipheight: %d\n", h);
				h = read_int (file);
				printf ("Clip X offset: %d\n", h);
				h = read_int (file);
				printf ("Clip Y offset: %d\n", h);
				h = read_int (file);
				printf ("Transparent colour: %08Xh\n", h);
				// That unknown integer again
				h = read_int (file);
				printf ("UnknownE: %08Xh (%d)\n", h, h);
				break;

			// The same, without that last integer:
			case 22:  // Sprite16
				// Blend info only if InfoByte is 3
				if (InfoByte == 3)
				{
					h = read_int (file);
					printf ("ShowMode: %08Xh", h);
					// Parse this into its components
					switch (h & 0x000000ff)
					{
						case 0: printf (" = smOpaque"); break;
						case 1: printf (" = smTransparent"); break;
						case 2: printf (" = smBlended, ");
							// Show extra blend info
							// Include unknown ones just for good measure
							switch ((h >> 8) & 0x000000ff)
							{
								case 0: printf ("bmUser"); break;
								case 1: printf ("bmAlpha"); break;
								case 2: printf ("bmBrighten"); break;
								case 3: printf ("bmIntensity"); break;
								case 4: printf ("bmShadow"); break;
								case 5: printf ("bmLinearAlpha"); break;
								default: printf ("Unknown");
							}
							break;
						default: printf (" -- Unknown");
					}
					printf ("\n");
					// This one is always there, although
					// only used with smBlended mode
					h = read_int (file);
					printf ("BlendValue: %d\n", h);
				}
				h = read_int (file);
				printf ("PixelFormat: %08Xh\n", h);
				h = read_int (file);
				printf ("Clipwidth: %d\n", h);
				h = read_int (file);
				printf ("Clipheight: %d\n", h);
				h = read_int (file);
				printf ("Clip X offset: %d\n", h);
				h = read_int (file);
				printf ("Clip Y offset: %d\n", h);
				h = read_int (file);
				printf ("Transparent colour: %08Xh\n", h);
				break;
		}

	// Test if we need to skip data;
	// this we needed to save the size for.
	// A simple file reposition will do.
		if (InfoByte == 1)
		{
			fseek (file, size, SEEK_CUR);
		}
		if (isComposite) continue;
	// We should be at the end of an image.
	// If not, something is wrong.
		h = read_int (file);
		if (h != 0xffffffff)
		{
			printf ("Unexpected end value %08Xh!\n", h);
			fclose (file);
			exit(EXIT_FAILURE);
		}
		printf ("\n");
	}  // End of the while loop

	fclose (file);
	printf ("Successfully ended!\n");
	exit(EXIT_SUCCESS);
}
