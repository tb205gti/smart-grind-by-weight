"""
Advanced flow rate analysis utilities for grinder performance reports
"""
import pandas as pd
import numpy as np
from typing import Dict, List, Optional, Tuple

def calculate_flow_rate_stats(measurements_df: pd.DataFrame) -> Dict:
    """Calculate comprehensive flow rate statistics for a session"""
    if measurements_df.empty:
        return {}
    
    # Filter out zero flow rates and motor-off periods for analysis
    active_measurements = measurements_df[
        (measurements_df['flow_rate_g_per_s'] > 0) & 
        (measurements_df['motor_is_on'] == 1)
    ]
    
    if active_measurements.empty:
        return {"error": "No active flow rate data found"}
    
    flow_rates = active_measurements['flow_rate_g_per_s']
    
    stats = {
        'mean_flow_rate': flow_rates.mean(),
        'max_flow_rate': flow_rates.max(),
        'min_flow_rate': flow_rates.min(),
        'std_flow_rate': flow_rates.std(),
        'median_flow_rate': flow_rates.median(),
        'total_active_time_ms': len(active_measurements) * 25,  # Assuming 25ms intervals
        'flow_rate_samples': len(flow_rates),
        'flow_rate_percentiles': {
            '25th': flow_rates.quantile(0.25),
            '75th': flow_rates.quantile(0.75),
            '90th': flow_rates.quantile(0.90),
            '95th': flow_rates.quantile(0.95)
        }
    }
    
    # Calculate flow rate stability (coefficient of variation)
    if stats['mean_flow_rate'] > 0:
        stats['flow_rate_cv'] = stats['std_flow_rate'] / stats['mean_flow_rate']
    else:
        stats['flow_rate_cv'] = float('inf')
    
    return stats

def detect_flow_phases(measurements_df: pd.DataFrame, min_phase_duration_ms: int = 200) -> List[Dict]:
    """
    Detect distinct flow phases in the grinding session
    Returns list of phases with start/end times and characteristics
    """
    if measurements_df.empty:
        return []
    
    # Sort by timestamp
    df = measurements_df.sort_values('timestamp_ms').copy()
    
    phases = []
    current_phase_start = None
    current_phase_type = None
    
    for idx, row in df.iterrows():
        is_motor_on = row['motor_is_on'] == 1
        flow_rate = row['flow_rate_g_per_s']
        timestamp = row['timestamp_ms']
        
        # Determine phase type
        if is_motor_on and flow_rate > 0.1:
            phase_type = 'grinding'
        elif is_motor_on and flow_rate <= 0.1:
            phase_type = 'motor_on_no_flow'
        else:
            phase_type = 'settling'
        
        # Check for phase transition
        if phase_type != current_phase_type:
            # End previous phase if it exists and meets minimum duration
            if current_phase_start is not None:
                phase_duration = timestamp - current_phase_start
                if phase_duration >= min_phase_duration_ms:
                    phases.append({
                        'type': current_phase_type,
                        'start_time_ms': current_phase_start,
                        'end_time_ms': timestamp,
                        'duration_ms': phase_duration
                    })
            
            # Start new phase
            current_phase_start = timestamp
            current_phase_type = phase_type
    
    # Close final phase
    if current_phase_start is not None:
        final_timestamp = df['timestamp_ms'].iloc[-1]
        phase_duration = final_timestamp - current_phase_start
        if phase_duration >= min_phase_duration_ms:
            phases.append({
                'type': current_phase_type,
                'start_time_ms': current_phase_start,
                'end_time_ms': final_timestamp,
                'duration_ms': phase_duration
            })
    
    return phases

def calculate_grind_efficiency(session_data: Dict, measurements_df: pd.DataFrame) -> Dict:
    """Calculate grinding efficiency metrics"""
    if measurements_df.empty:
        return {}
    
    total_time_ms = session_data.get('total_time_s', 0) * 1000
    motor_on_time_ms = session_data.get('total_motor_on_time_ms', 0)
    final_weight = session_data.get('final_weight', 0)
    
    # Calculate weight added (assuming tare at start)
    if not measurements_df.empty:
        min_weight = measurements_df['weight_grams'].min()
        weight_added = final_weight - min_weight
    else:
        weight_added = final_weight
    
    efficiency_metrics = {
        'motor_duty_cycle': motor_on_time_ms / total_time_ms if total_time_ms > 0 else 0,
        'grind_rate_g_per_s': weight_added / (total_time_ms / 1000) if total_time_ms > 0 else 0,
        'motor_efficiency_g_per_s': weight_added / (motor_on_time_ms / 1000) if motor_on_time_ms > 0 else 0,
        'weight_added_grams': weight_added,
        'total_time_s': total_time_ms / 1000,
        'motor_on_time_s': motor_on_time_ms / 1000
    }
    
    return efficiency_metrics

def smooth_flow_rate(measurements_df: pd.DataFrame, window_ms: int = 500) -> pd.DataFrame:
    """Apply time-based smoothing to flow rate data"""
    if measurements_df.empty:
        return measurements_df
    
    df = measurements_df.copy().sort_values('timestamp_ms')
    
    # Convert to datetime for rolling window
    df['timestamp_dt'] = pd.to_datetime(df['timestamp_ms'], unit='ms')
    df_indexed = df.set_index('timestamp_dt')
    
    # Apply rolling mean with time-based window
    window_str = f"{window_ms}ms"
    df_indexed['flow_rate_smoothed'] = df_indexed['flow_rate_g_per_s'].rolling(
        window=window_str, min_periods=1
    ).mean()
    
    # Merge back to original dataframe
    df['flow_rate_smoothed'] = df_indexed['flow_rate_smoothed'].values
    
    return df.drop(columns=['timestamp_dt'])

def compare_sessions(sessions_list: List[Dict], measurements_dict: Dict[int, pd.DataFrame]) -> Dict:
    """Compare multiple grinding sessions for performance analysis"""
    if not sessions_list:
        return {}
    
    comparison_data = []
    
    for session in sessions_list:
        session_id = session['session_id']
        measurements = measurements_dict.get(session_id, pd.DataFrame())
        
        flow_stats = calculate_flow_rate_stats(measurements)
        efficiency = calculate_grind_efficiency(session, measurements)
        
        comparison_data.append({
            'session_id': session_id,
            'target_weight': session.get('target_weight', 0),
            'final_weight': session.get('final_weight', 0),
            'error_grams': session.get('error_grams', 0),
            'pulse_count': session.get('pulse_count', 0),
            'total_time_s': session.get('total_time_s', 0),
            'mean_flow_rate': flow_stats.get('mean_flow_rate', 0),
            'max_flow_rate': flow_stats.get('max_flow_rate', 0),
            'flow_rate_cv': flow_stats.get('flow_rate_cv', 0),
            'motor_efficiency': efficiency.get('motor_efficiency_g_per_s', 0),
            'motor_duty_cycle': efficiency.get('motor_duty_cycle', 0)
        })
    
    comparison_df = pd.DataFrame(comparison_data)
    
    return {
        'comparison_data': comparison_df,
        'summary_stats': {
            'avg_error': comparison_df['error_grams'].mean(),
            'avg_pulse_count': comparison_df['pulse_count'].mean(),
            'avg_flow_rate': comparison_df['mean_flow_rate'].mean(),
            'avg_motor_efficiency': comparison_df['motor_efficiency'].mean(),
            'error_std': comparison_df['error_grams'].std(),
            'best_session_id': comparison_df.loc[comparison_df['error_grams'].abs().idxmin(), 'session_id'],
            'worst_session_id': comparison_df.loc[comparison_df['error_grams'].abs().idxmax(), 'session_id']
        }
    }