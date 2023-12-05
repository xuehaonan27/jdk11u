# Welcome to the JDK!

For build instructions please see the
[online documentation](https://openjdk.java.net/groups/build/doc/building.html),
or either of these files:

- [doc/building.html](doc/building.html) (html version)
- [doc/building.md](doc/building.md) (markdown version)

See <https://openjdk.java.net/> for more information about
the OpenJDK Community and the JDK.

## New GCs
### Full heap parallel mark compact collector
```bash
-Xms32g -Xmx32g -XX:+UseParallelGC -XX:+UseParallelFullMarkCompactGC -XX:NewSize=1k -XX:MaxNewSize=1k -XX:-UseAdaptiveSizePolicy
```
Parallel GC now use single thread to process `java.lang.ref.Reference` instances during full gc by default.  
For cache app, for example, QuickCached, that uses soft references, it will improve this phase significantly if parallel reference processing is enabled by specifying `-XX:+ParallelRefProcEnabled`. The ergonomics of `java.lang.ref.Reference` processing can be tuned by using the experimental option `-XX:ReferencesPerThread` (default value: 1000), and it starts one thread for this amount of references.
### Full heap parallel scavenge collector
```bash
-Xms32g -Xmx32g -XX:+UseParallelGC -XX:+UseParallelFullScavengeGC -XX:NewSize=32g -XX:MaxNewSize=32g -XX:SurvivorRatio=1 -XX:-UseAdaptiveSizePolicy
```
