# Portal Animation Fix Summary

## Issue
Portals were showing as static images instead of animating through their frames.

## Root Cause
In `Graphics/Animation.cpp`, the `update()` function was incorrectly using the `frame` object directly in comparisons and arithmetic operations instead of calling `frame.get()` to get the actual frame value.

## Solution
Updated the frame advancement logic to properly use `frame.get()`:

```cpp
// Fixed in Animation::update()
if (zigzag && lastframe > 0)
{
    if (framestep == 1 && frame.get() == lastframe)
    {
        framestep = -framestep;
        ended = false;
    }
    else if (framestep == -1 && frame.get() == 0)
    {
        framestep = -framestep;
        ended = true;
    }
    else
    {
        ended = false;
    }

    nextframe = frame.get() + framestep;
}
else
{
    if (frame.get() == lastframe)
    {
        nextframe = 0;
        ended = true;
    }
    else
    {
        nextframe = frame.get() + 1;
        ended = false;
    }
}
```

## Result
Portal animations now properly cycle through their frames, creating the expected animated effect for v92 portals.

## Files Modified
- `Graphics/Animation.cpp` - Fixed frame advancement logic in the `update()` method