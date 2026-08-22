namespace IOTProjekat.DTOs
{
    public class AddSensorDataDto
    {
        public decimal Temperature { get; set; }
        public decimal Humidity { get; set; }
        public decimal Light { get; set; }
        public string Status { get; set; }
        public DateTime Timestamp { get; set; }
        
    }
}
