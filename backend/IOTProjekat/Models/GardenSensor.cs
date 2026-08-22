using MongoDB.Bson.Serialization.Attributes;

namespace IOTProjekat.Models
{

    public class GardenSensor
    {
        [BsonId]
        [BsonRepresentation(MongoDB.Bson.BsonType.ObjectId)]
        public string Id { get; set; }
        public decimal Light { get; set; }
        public decimal Temperature { get; set; }
        public decimal AirHumidity { get; set; }
        public string Status { get; set; }
        public DateTime Time { get; set; }
    }
}
