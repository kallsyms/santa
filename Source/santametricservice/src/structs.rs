use std::collections::HashMap;
use chrono::{DateTime, Utc};
use serde::{Serialize, Deserialize};

#[derive(Debug, Serialize, Deserialize)]
#[serde(untagged)]
pub enum MetricValueData {
    Number(f64),
    String(String),
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MetricValue {
    pub created: DateTime<Utc>, 
    pub data: MetricValueData,
    pub last_updated: DateTime<Utc>,
    pub value: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Metric {
    pub description: String,
    pub fields: HashMap<String, Vec<MetricValue>>,
    pub r#type: i8,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MetricSet {
    pub metrics: HashMap<String, Metric>,
}
