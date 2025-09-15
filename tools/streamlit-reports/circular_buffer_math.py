"""
Python implementation of the CircularBufferMath 95th percentile flow rate calculation
that exactly matches the C++ implementation in circular_buffer_math.cpp
"""

import numpy as np
import pandas as pd
from typing import List, Tuple, Optional

# Hardware constants - must match hardware_config.h
HW_LOADCELL_SAMPLE_RATE_SPS = 10  # Current sample rate setting

def calculate_95th_percentile_flow_rate(measurements_df: pd.DataFrame, 
                                       current_timestamp_ms: int, # This is the 'now' timestamp
                                       window_ms: int = 200,
                                       sub_window_ms: Optional[int] = None,
                                       step_ms: Optional[int] = None) -> float:
    """
    Python implementation that exactly matches CircularBufferMath::get_raw_flow_rate_95th_percentile()
    
    Args:
        measurements_df: DataFrame with 'timestamp_ms' and flow rate columns
        current_timestamp_ms: The current timestamp to calculate from (simulates millis())
        window_ms: Time window in milliseconds (default 200ms like C++)
        sub_window_ms: Sub-window size in ms (if None, uses C++ fraction approach)
        step_ms: Step size in ms (if None, uses C++ fraction approach)
    
    Returns:
        95th percentile flow rate in g/s (converted from raw units)
    """
    # Define parameters to match the C++ implementation
    MIN_SAMPLES_FOR_PERCENTILE = 10
    # Use provided sub_window/step or default to C++ constants
    SUB_WINDOW_MS = sub_window_ms if sub_window_ms is not None else 300
    STEP_MS = step_ms if step_ms is not None else 100
    MIN_SUB_WINDOWS = 4
    MAX_SUB_WINDOWS = 32
    MIN_SAMPLES_PER_SUB_WINDOW = 3

    if len(measurements_df) < MIN_SAMPLES_FOR_PERCENTILE:
        return calculate_simple_flow_rate(measurements_df, current_timestamp_ms, window_ms) # Fallback
    
    # Ensure at least ~10 samples in the window based on hardware SPS
    min_window_for_samples = (MIN_SAMPLES_FOR_PERCENTILE * 1000) // HW_LOADCELL_SAMPLE_RATE_SPS
    effective_window_ms = max(window_ms, min_window_for_samples)

    # 1. Collect all relevant samples in one go (from newest to oldest)
    window_start_time = current_timestamp_ms - effective_window_ms
    collected_samples_df = measurements_df[
        (measurements_df['timestamp_ms'] >= window_start_time) &
        (measurements_df['timestamp_ms'] <= current_timestamp_ms)
    ].sort_values('timestamp_ms', ascending=False)

    if len(collected_samples_df) < MIN_SAMPLES_FOR_PERCENTILE:
        return calculate_simple_flow_rate(measurements_df, current_timestamp_ms, effective_window_ms)

    # Convert to numpy for much faster access
    sample_values = collected_samples_df['weight_grams'].values
    sample_times = collected_samples_df['timestamp_ms'].values

    # 2. Calculate the number of sub-windows
    num_sub_windows = 1 + (max(0, effective_window_ms - SUB_WINDOW_MS) // STEP_MS)
    num_sub_windows = max(MIN_SUB_WINDOWS, min(MAX_SUB_WINDOWS, num_sub_windows))

    flow_rates = []

    # 3. Iterate through sub-windows and calculate flow rate for each
    for i in range(num_sub_windows):
        sub_window_end_time = current_timestamp_ms - (i * STEP_MS)
        sub_window_start_time = sub_window_end_time - SUB_WINDOW_MS

        # Find indices of samples within this sub-window (fast with numpy)
        in_window_indices = np.where((sample_times <= sub_window_end_time) & (sample_times >= sub_window_start_time))[0]

        if len(in_window_indices) >= MIN_SAMPLES_PER_SUB_WINDOW:
            newest_idx = in_window_indices.min() # Smallest index is newest time
            oldest_idx = in_window_indices.max() # Largest index is oldest time

            time_delta = sample_times[newest_idx] - sample_times[oldest_idx]
            if time_delta > 0:
                # Correct calculation: (newest - oldest)
                raw_delta = sample_values[newest_idx] - sample_values[oldest_idx]
                flow_rate_per_sec = raw_delta * 1000.0 / time_delta
                flow_rates.append(flow_rate_per_sec)

    # 4. Calculate the 95th percentile
    if len(flow_rates) >= MIN_SAMPLES_PER_SUB_WINDOW:
        # Sort and take the 95th percentile (matches C++ exactly)
        flow_rates.sort()
        percentile_95_index = int(len(flow_rates) * 0.95)
        if percentile_95_index >= len(flow_rates):
            percentile_95_index = len(flow_rates) - 1
        return flow_rates[percentile_95_index]
    
    # Fallback if we couldn't get enough valid sub-window rates
    return calculate_simple_flow_rate(measurements_df, current_timestamp_ms, effective_window_ms)


def calculate_simple_flow_rate(measurements_df: pd.DataFrame, 
                              current_timestamp_ms: int,
                              window_ms: int) -> float:
    """
    Simple flow rate calculation (matches get_raw_flow_rate in C++)
    """
    window_start = current_timestamp_ms - window_ms
    
    window_data = measurements_df[
        (measurements_df['timestamp_ms'] >= window_start) & 
        (measurements_df['timestamp_ms'] <= current_timestamp_ms)
    ].sort_values('timestamp_ms')
    
    if len(window_data) < 2:
        return 0.0
    
    # Simple linear regression for flow rate (most recent - oldest)
    first_row = window_data.iloc[0]  # Oldest
    last_row = window_data.iloc[-1]  # Most recent
    
    raw_change = float(last_row['weight_grams'] - first_row['weight_grams'])
    time_change = float(last_row['timestamp_ms'] - first_row['timestamp_ms'])
    
    if time_change == 0:
        return 0.0
    
    # Return g per second
    return raw_change * 1000.0 / time_change


def calculate_95th_percentile_series(measurements_df: pd.DataFrame, 
                                   window_ms: int = 200,
                                   sub_window_ms: Optional[int] = None,
                                   step_ms: Optional[int] = None) -> pd.DataFrame:
    """
    Calculate 95th percentile flow rate for each measurement timestamp.
    This simulates calling the C++ function at each measurement point.
    
    Args:
        measurements_df: DataFrame with 'timestamp_ms' and 'weight_grams'
        window_ms: Time window in milliseconds
        sub_window_ms: Sub-window size in ms (if None, uses C++ fraction approach)
        step_ms: Step size in ms (if None, uses C++ fraction approach)
    
    Returns:
        DataFrame with original data plus 'flow_rate_95p_cpp' column
    """
    if measurements_df.empty:
        return measurements_df
    
    # Sort once and copy to avoid modifying the original DataFrame
    df = measurements_df.sort_values('timestamp_ms').copy()
    
    # OPTIMIZED: Use pandas .apply() for a vectorized-like operation.
    # This is much faster than iterating row-by-row in Python.
    # The lambda function simulates calling the C++ function at each point in time.
    df['flow_rate_95p_cpp'] = df.apply(
        lambda row: calculate_95th_percentile_flow_rate(
            # Pass only the data available up to the current row's timestamp
            measurements_df=df[df['timestamp_ms'] <= row['timestamp_ms']],
            current_timestamp_ms=int(row['timestamp_ms']),
            window_ms=window_ms,
            sub_window_ms=sub_window_ms,
            step_ms=step_ms
        ),
        axis=1
    )

    return df