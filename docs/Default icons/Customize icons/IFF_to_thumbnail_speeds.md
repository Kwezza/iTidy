# IFF to Thumbnail Render Performance

## Test Configuration

- **Hardware**: Stock 68000 @ ~7MHz (WinUAE emulation)
- **OS**: Workbench 3.2
- **Thumbnail Size**: 64×64 pixels
- **Test Date**: 2026-02-11
- **Log File**: `icons_2026-02-11_15-02-20.log`

## Performance Results

### Standard Lowres Images (320×200)

All standard lowres images were 5 bitplanes (32 colors) with no special display modes.

| Image | Resolution | Bitplanes | Decode Time | Render Time | Total Time |
|-------|-----------|-----------|-------------|-------------|------------|
| Yacht | 320×200 | 5 | ~13s | ~7s | ~20s |
| Waterfall | 320×200 | 5 | ~13s | ~7s | ~20s |
| Venus | 320×200 | 5 | ~13s | ~7s | ~20s |
| Space | 320×200 | 5 | ~13s | ~7s | ~20s |

**Average**: ~20 seconds per thumbnail

### Hi-Res Interlaced Images

Larger images with hi-res and/or interlaced display modes.

| Image | Resolution | Bitplanes | Display Mode | Decode Time | Render Time | Total Time |
|-------|-----------|-----------|--------------|-------------|-------------|------------|
| Zebra-Stripes-V2.IFF | 672×446 | 4 | Hi-res + Interlace | ~50s | ~17s | ~67s |
| Utopia-17.IFF | 336×442 | 5 | Interlace | ~30s | ~12s | ~42s |

**Range**: 42-67 seconds per thumbnail

### HAM Images (Datatype Fallback)

HAM (Hold-And-Modify) images require special handling and use the datatype fallback path with full RGB24 quantization.

| Image | Resolution | Bitplanes | Display Mode | Decode Time | Quantization Time | Render Time | Total Time |
|-------|-----------|-----------|--------------|-------------|-------------------|-------------|------------|
| Utopia-16.IFF | 336×442 | 6 | HAM6 + Interlace | ~4s | ~24min 3s | ~34s | ~24min 36s |

**HAM Performance**: ~25 minutes per thumbnail (!)

## Analysis

### Native IFF Parser Performance
The native IFF parser (used for standard and interlaced images) provides acceptable performance:
- **Lowres (320×200)**: ~20 seconds - practical for interactive use
- **Hi-res/Interlaced**: 40-67 seconds - slower but acceptable for batch operations

### HAM Datatype Fallback Performance
The datatype fallback for HAM images is extremely slow on stock 68000 hardware:
- **RGB24 Quantization**: The 24-bit to 8-bit palette quantization takes ~24 minutes
- **Root Cause**: Full RGB color space processing in software on a 7MHz 68000
- **Recommendation**: HAM thumbnails are impractical on stock 68000 systems

### Performance Scaling
Render time correlates with:
1. **Image dimensions** (pixel count)
2. **Bitplane count** (memory bandwidth)
3. **Display mode** (hi-res/interlace adds overhead)
4. **Color space** (HAM requires special handling)

## Recommendations

### For Stock 68000 Systems:
- ✅ Standard lowres images: Acceptable (~20s each)
- ✅ Hi-res/interlaced images: Acceptable for batch operations (~40-60s each)
- ❌ HAM images: Too slow for practical use (~25 minutes each)

### For Accelerated Systems:
- With a 68020+ and Fast RAM, expect 10-15× performance improvement
- HAM images would become practical (potentially under 2 minutes)

### Optimization Opportunities:
1. Skip HAM images on stock 68000 systems (or provide user option)
2. Consider lower quality/faster quantization for HAM fallback
3. Add progress indicators for images expected to take >30 seconds
4. Batch processing with estimates based on image format detection

## Technical Notes

- **Native Parser**: Handles standard ILBM formats efficiently
- **Datatype Fallback**: Required for HAM, uses 256-color palette (6×6×6 cube + grays)
- **Memory Usage**: All processing done in Fast RAM when available
- **Compression**: All test images used ByteRun1 (RLE) compression
