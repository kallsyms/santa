use crate::structs::*;
use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub enum Value {
    #[serde(rename = "boolValue")]
    Bool(bool),
    #[serde(rename = "int64Value")]
    I64(i64),
    #[serde(rename = "doubleValue")]
    Double(f64),
    #[serde(rename = "stringValue")]
    String(String),
}

#[derive(Debug, Serialize, Deserialize)]
pub struct FieldValue {
    name: String,
    #[serde(flatten)]
    value: Value,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MetricData {
    // TODO: do we need to ensure these only go to 3 digits after the decimal?
    #[serde(rename = "startTimestamp")]
    start_timestamp: DateTime<Utc>,
    #[serde(rename = "endTimestamp")]
    end_timestamp: DateTime<Utc>,
    field: Option<Vec<FieldValue>>,
    #[serde(flatten)]
    value: Value,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct FieldDescriptor {
    name: String,
    #[serde(rename = "fieldType")]
    field_type: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MetricsData {
    #[serde(rename = "metricName")]
    metric_name: String,
    description: String,
    #[serde(rename = "fieldDescriptor")]
    field_descriptor: Vec<FieldDescriptor>,
    #[serde(rename = "valueType")]
    value_type: String,
    #[serde(rename = "streamKind")]
    stream_kind: String,
    data: Vec<MetricData>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MetricsCollection {
    #[serde(rename = "metricsDataSet")]
    dataset: Vec<MetricsData>,
    //rootLabels
}

#[derive(Debug, Serialize, Deserialize)]
pub struct CollectionWrapper {
    #[serde(rename = "metricsCollection")]
    collection: MetricsCollection,
}

fn encode_data(metric: &Metric) -> Vec<MetricData> {
    let mut datas = vec![];
    for (field_name, entries) in &metric.fields {
        for entry in entries {
            // TODO: there's probably a better way to do this
            let mut number_value: Option<f64> = None;
            let mut string_value: Option<String> = None;
            match &entry.data {
                MetricValueData::Number(n) => number_value = Some(*n),
                MetricValueData::String(s) => string_value = Some(s.clone()),
            }

            let data = MetricData {
                start_timestamp: entry.created,
                end_timestamp: Utc::now(),
                field: if field_name == "" {
                    None
                } else {
                    Some(field_name
                        .split(",")
                        .zip(entry.value.split(","))
                        .map(|(name, value)| FieldValue {
                            name: name.to_string(),
                            value: Value::String(value.to_string()),
                        })
                        .collect())
                },
                value: match metric.r#type {
                    1 | 5 => Value::Bool(number_value.unwrap() as i64 != 0),
                    2 | 6 => Value::String(string_value.unwrap()),
                    3 | 7 => Value::I64(number_value.unwrap() as i64),
                    4 | 8 => Value::Double(number_value.unwrap()),
                    9 => Value::I64(number_value.unwrap() as i64),
                    _ => panic!("Invalid metric type {}", metric.r#type),
                },
            };

            datas.push(data);
        }
    }
    datas
}

fn encode_field_descriptors(metric: &Metric) -> Vec<FieldDescriptor> {
    let mut fds = vec![];
    for field in metric.fields.keys() {
        if field != "" {
            for field_name in field.split(",") {
                fds.push(FieldDescriptor {
                    name: field_name.to_string(),
                    field_type: "STRING".to_string(),
                });
            }
        }
    }
    fds
}

fn format_metric(name: &str, metric: &Metric) -> MetricsData {
    MetricsData {
        metric_name: name.to_string(),
        description: metric.description.clone(),
        field_descriptor: encode_field_descriptors(metric),
        value_type: match metric.r#type {
            1 | 5 => "BOOL",
            2 | 6 => "STRING",
            3 | 7 => "INT64",
            4 | 8 => "DOUBLE",
            9 => "INT64",
            _ => panic!("Invalid metric type {}", metric.r#type),
        }
        .to_string(),
        stream_kind: match metric.r#type {
            1 | 2 | 3 | 4 => "",
            5 | 6 | 7 | 8 => "GAUGE",
            9 => "CUMULATIVE",
            _ => panic!("Invalid metric type {}", metric.r#type),
        }
        .to_string(),
        data: encode_data(metric),
    }
}

pub fn convert(metrics: &MetricSet) -> CollectionWrapper {
    CollectionWrapper {
        collection: MetricsCollection {
            dataset: metrics
                .metrics
                .iter()
                .map(|(k, v)| format_metric(k, v))
                .collect(),
        },
    }
}
