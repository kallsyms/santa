use crate::structs::*;
use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub enum Value {
    boolValue(bool),
    int64Value(i64),
    doubleValue(f64),
    stringValue(String),
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
    startTimestamp: DateTime<Utc>,
    endTimestamp: DateTime<Utc>,
    field: Option<Vec<FieldValue>>,
    #[serde(flatten)]
    value: Value,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct FieldDescriptor {
    name: String,
    fieldType: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MetricsData {
    metricName: String,
    description: String,
    fieldDescriptor: Vec<FieldDescriptor>,
    valueType: String,
    streamKind: String,
    data: Vec<MetricData>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MetricsCollection {
    metricsDataSet: Vec<MetricsData>,
    //rootLabels
}

#[derive(Debug, Serialize, Deserialize)]
pub struct CollectionWrapper {
    metricsCollection: MetricsCollection,
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
                startTimestamp: entry.created,
                endTimestamp: Utc::now(),
                field: if field_name == "" {
                    None
                } else {
                    Some(field_name
                        .split(",")
                        .zip(entry.value.split(","))
                        .map(|(name, value)| FieldValue {
                            name: name.to_string(),
                            value: Value::stringValue(value.to_string()),
                        })
                        .collect())
                },
                value: match metric.r#type {
                    1 | 5 => Value::boolValue(number_value.unwrap() as i64 != 0),
                    2 | 6 => Value::stringValue(string_value.unwrap()),
                    3 | 7 => Value::int64Value(number_value.unwrap() as i64),
                    4 | 8 => Value::doubleValue(number_value.unwrap()),
                    9 => Value::int64Value(number_value.unwrap() as i64),
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
                    fieldType: "STRING".to_string(),
                });
            }
        }
    }
    fds
}

fn format_metric(name: &str, metric: &Metric) -> MetricsData {
    MetricsData {
        metricName: name.to_string(),
        description: metric.description.clone(),
        fieldDescriptor: encode_field_descriptors(metric),
        valueType: match metric.r#type {
            1 | 5 => "BOOL",
            2 | 6 => "STRING",
            3 | 7 => "INT64",
            4 | 8 => "DOUBLE",
            9 => "INT64",
            _ => panic!("Invalid metric type {}", metric.r#type),
        }
        .to_string(),
        streamKind: match metric.r#type {
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
        metricsCollection: MetricsCollection {
            metricsDataSet: metrics
                .metrics
                .iter()
                .map(|(k, v)| format_metric(k, v))
                .collect(),
        },
    }
}
