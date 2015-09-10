durationMs = Math.round(duration * 1000);
rounded = durationMs / 10.0;
result = (Math.floor(rounded) + (((rounded) < (Math.floor(rounded) + 0.5)) ? 0 : 0.5)) / 100;
table.printf("%.3f", result);