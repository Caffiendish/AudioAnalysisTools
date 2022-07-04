This is a dump of a WIP runtime usable Synesthesia plugin.

It's the absolute start of one, but what I'd recommend to anyone who continues to work on this, 
is to inherit the basic Synesthesia classes, like UOnsetNRT, and override all the functions that call GetResult.

There's no reason to do FFT calculations every function call, when we could "bake" on import/request, and then just return that data.

I'd also say to inherit UAudioComponent, make your own component, and override the "Get FFT" functions, take the basic Synesthesia classes as data,
and then use either custom or default Synesthesia functions to provide data in a manner akin to the current FFT functions,
which, if they actually worked, would be incredibly helpful, since you can just call that on the tick.

## Legal info

Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.

Unreal® Engine, Copyright 1998 – 2022, Epic Games, Inc. All rights reserved.
