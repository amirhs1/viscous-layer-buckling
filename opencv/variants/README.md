# Pipeline variants (not built)

These are experimental versions of pipeline stages, kept for reference. The top-level
`CMakeLists.txt` does not build them; each folder has its original `CMakeLists.txt`.

| Folder | Variant of | Notes |
|---|---|---|
| `template_matching_taylor_length/` | stage 2 | Originally `TemplateMatching-Taylor`. It is the thesis-appendix listing plus an unfinished Taylor-length parameter (`const double tau = ;`), so it does not compile |
| `interface_statistics_big_curvature/` | stage 3 | Originally `Statis-Big-Curvature`; differs from stage 3 in 269 lines |
| `interface_statistics_moving_average_only/` | stage 3 | Moving-average smoothing only, no B-spline |
