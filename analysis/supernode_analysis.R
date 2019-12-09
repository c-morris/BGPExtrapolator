library(ggplot2)
library(dplyr)

# Load data
data <- read.csv('supernodes.csv', header = FALSE)
colnames(data) <- c('as', 'component')
components_data <- data %>% group_by(component) %>% tally() %>% filter(n>1)
colnames(components_data) <- c('component', 'component_size')
plot_obj <- ggplot(components_data, aes(component_size)) + geom_histogram()
print(plot_obj)