use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Serialize, Deserialize)]
#[serde(untagged)]
pub enum MetricValueData {
    Bool(bool),
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
    pub root_labels: HashMap<String, String>, // TODO: should this be String, MetricValueData?
}
