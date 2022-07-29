Font generating tool, extracts font outline cubic curves from selected Windows font.

Output file format:

```
typedef struct
{
	uint32_t Advance;		// Character advancement width
	uint32_t numPath;		// Number of vectors in Path
	vec2 *Path;				// 2D path vectors that make up cubic Bezier curves
} Gylph_t;

typedef struct
{
	uint32_t Magic;			// "GYLP"
	uint32_t Size;			// Font character height
	Gylph_t Gylphs[256];	// Character gylphs
} Font_t;
```

To-do:
Doesn't close outline, works fine with single stroke fonts, but will have an open path on traditional outline fonts.
